
#include "Commands.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include "Parser.hpp"
#include "Utils.hpp"
#include "Replies.hpp"
#include <sstream>
#include <cstdlib>

static std::string prefixFor(Server &srv, Client *c) {
    // :nick!user@server
    std::string nick = c->nick().empty()? "*": c->nick();
    std::string user = c->user().empty()? "user": c->user();
    return ":" + nick + "!" + user + "@" + srv.serverName() + " ";
}

static bool ensureRegistered(Server &srv, Client *c) {
    // Registration finalization: PASS ok + NICK + USER set
    if (!c->registered() && c->passOk() && !c->nick().empty() && !c->user().empty()) {
        c->setRegistered(true);
        srv.sendToClient(c->fd(), RPL::welcome(srv.serverName(), c->nick()));
        return true;
    }
    return c->registered();
}

// PASS <password>
void CMD::PASS(Server &srv, int fd, const std::vector<std::string> &p) {
    Client *c = srv.getClient(fd);
    if (!c) return;
    if (c->registered()) {
        srv.sendToClient(fd, ERR::alreadyreg(srv.serverName(), c->nick()));
        return;
    }
    if (p.size() < 1) {
        srv.sendToClient(fd, ERR::needmoreparams(srv.serverName(), c->nick(), "PASS"));
        return;
    }
    if (p[0] == srv.password()) c->setPassOk(true);
    else srv.sendToClient(fd, ERR::passmismatch(srv.serverName(), c->nick()));
    ensureRegistered(srv, c);
}

// NICK <nick>
void CMD::NICK(Server &srv, int fd, const std::vector<std::string> &p) {
    Client *c = srv.getClient(fd);
    if (!c) return;
    if (p.size() < 1) {
        srv.sendToClient(fd, ERR::needmoreparams(srv.serverName(), c->nick(), "NICK"));
        return;
    }
    std::string newNick = p[0];
    if (!isValidNick(newNick)) {
        // Very basic: treat invalid as in use.
        srv.sendToClient(fd, ERR::nicknameinuse(srv.serverName(), newNick));
        return;
    }
    // Uniqueness check (case-insensitive).
    if (srv.nickToFd().count(toLower(newNick))) {
        srv.sendToClient(fd, ERR::nicknameinuse(srv.serverName(), newNick));
        return;
    }
    // Remove old mapping if existed.
    if (!c->nick().empty()) srv.nickToFd().erase(toLower(c->nick()));
    c->setNick(newNick);
    srv.nickToFd()[toLower(newNick)] = fd;
    ensureRegistered(srv, c);
}

// USER <user> 0 * :realname
void CMD::USER(Server &srv, int fd, const std::vector<std::string> &p) {
    Client *c = srv.getClient(fd);
    if (!c) return;
    if (c->registered()) {
        srv.sendToClient(fd, ERR::alreadyreg(srv.serverName(), c->nick()));
        return;
    }
    if (p.size() < 4) {
        srv.sendToClient(fd, ERR::needmoreparams(srv.serverName(), c->nick(), "USER"));
        return;
    }
    c->setUser(p[0]);
    c->setReal(p[3]);
    ensureRegistered(srv, c);
}

// JOIN <#chan>[ key]
void CMD::JOIN(Server &srv, int fd, const std::vector<std::string> &p) {
    Client *c = srv.getClient(fd);
    if (!c || !c->registered()) return;
    if (p.size() < 1) {
        srv.sendToClient(fd, ERR::needmoreparams(srv.serverName(), c->nick(), "JOIN"));
        return;
    }
    std::string chan = p[0];
    if (chan.empty() || chan[0] != '#') chan = "#" + chan;
    std::string key = p.size() >= 2 ? p[1] : "";

    Channel *ch = srv.getOrCreateChannel(toLower(chan));
    // Enforce +i, +k, +l
    if (ch->inviteOnly() && !ch->isInvited(fd)) {
        srv.sendToClient(fd, ERR::inviteonlychan(srv.serverName(), c->nick(), chan));
        return;
    }
    if (ch->hasKey() && ch->key() != key) {
        srv.sendToClient(fd, ERR::badchannelkey(srv.serverName(), c->nick(), chan));
        return;
    }
    if (ch->hasLimit() && ch->memberCount() >= ch->limit()) {
        srv.sendToClient(fd, ERR::channelisfull(srv.serverName(), c->nick(), chan));
        return;
    }
    // First user becomes operator.
    bool wasEmpty = ch->memberCount() == 0;
    ch->addMember(fd);
    if (wasEmpty) ch->addOperator(fd);
    ch->clearInvite(fd);

    // Broadcast JOIN
    std::string pre = prefixFor(srv, c);
    std::string joinLine = pre + "JOIN :" + chan + "\r\n";
    srv.sendToChannel(chan, fd, joinLine);
    srv.sendToClient(fd, joinLine);

    // Send topic and names
    if (!ch->topic().empty())
        srv.sendToClient(fd, RPL::topic(srv.serverName(), c->nick(), chan, ch->topic()));
    else
        srv.sendToClient(fd, RPL::notopic(srv.serverName(), c->nick(), chan));

    // Build names list with '@' for ops.
    std::string names;
    const std::set<int> &m = ch->members();
    for (std::set<int>::const_iterator it = m.begin(); it != m.end(); ++it) {
        Client *mc = srv.getClient(*it);
        if (!mc) continue;
        if (names.size()) names += " ";
        bool isOp = ch->isOperator(*it);
        names += (isOp? "@":"") + mc->nick();
    }
    srv.sendToClient(fd, RPL::namreply(srv.serverName(), c->nick(), chan, names));
    srv.sendToClient(fd, RPL::endofnames(srv.serverName(), c->nick(), chan));
}


// PART <#chan>[,#chan2...] [:reason]
void CMD::PART(Server &srv, int fd, const std::vector<std::string> &p) {
    Client *c = srv.getClient(fd);
    if (!c || !c->registered()) return;
    if (p.size() < 1) {
        srv.sendToClient(fd, ERR::needmoreparams(srv.serverName(), c->nick(), "PART"));
        return;
    }
    std::string chanlist = p[0];
    std::string reason = (p.size() >= 2) ? p[1] : "Leaving";
    std::vector<std::string> chans = split(chanlist, ',');
    for (size_t i = 0; i < chans.size(); ++i) {
        std::string chan = chans[i];
        if (chan.empty()) continue;
        if (chan[0] != '#') chan = "#" + chan;
        Channel *ch = srv.findChannel(toLower(chan));
        if (!ch) {
            srv.sendToClient(fd, ERR::nosuchchannel(srv.serverName(), c->nick(), chan));
            continue;
        }
        if (!ch->isMember(fd)) {
            srv.sendToClient(fd, ERR::notonchannel(srv.serverName(), c->nick(), chan));
            continue;
        }
        std::string pre = prefixFor(srv, c);
        std::string line = pre + "PART " + chan + " :" + reason + "\r\n";
        // Notify others and self
        srv.sendToChannel(chan, fd, line);
        srv.sendToClient(fd, line);
        // Remove membership and maybe destroy empty channel
        ch->removeMember(fd);
        if (ch->members().empty()) srv.removeChannelIfEmpty(ch->name());
    }
}
// PRIVMSG <target> :text
void CMD::PRIVMSG(Server &srv, int fd, const std::vector<std::string> &p) {
    Client *c = srv.getClient(fd);
    if (!c || !c->registered()) return;
    if (p.size() < 2) {
        srv.sendToClient(fd, ERR::needmoreparams(srv.serverName(), c->nick(), "PRIVMSG"));
        return;
    }
    std::string target = p[0];
    std::string text = p[1];
    std::string line = prefixFor(srv, c) + "PRIVMSG " + target + " :" + text + "\r\n";
    if (!target.empty() && target[0] == '#') {
        Channel *ch = srv.findChannel(toLower(target));
        if (!ch) {
            srv.sendToClient(fd, ERR::nosuchchannel(srv.serverName(), c->nick(), target));
            return;
        }
        if (!ch->isMember(fd)) {
            srv.sendToClient(fd, ERR::notonchannel(srv.serverName(), c->nick(), target));
            return;
        }
        srv.sendToChannel(target, fd, line);
    } else {
        Client *dst = srv.getClientByNick(target);
        if (!dst) {
            srv.sendToClient(fd, ERR::nosuchnick(srv.serverName(), c->nick(), target));
            return;
        }
        srv.sendToClient(dst->fd(), line);
    }
}

// MODE <#chan> +/-[itkol] [args...]
void CMD::MODE(Server &srv, int fd, const std::vector<std::string> &p) {
    Client *c = srv.getClient(fd);
    if (!c || !c->registered()) return;
    if (p.size() < 1) {
        srv.sendToClient(fd, ERR::needmoreparams(srv.serverName(), c->nick(), "MODE"));
        return;
    }
    std::string chan = p[0];
    if (chan.empty() || chan[0] != '#') chan = "#" + chan;
    Channel *ch = srv.findChannel(toLower(chan));
    if (!ch) {
        srv.sendToClient(fd, ERR::nosuchchannel(srv.serverName(), c->nick(), chan));
        return;
    }
    // If only viewing, just return modes (super minimal: we won't print in RFC style; not required).
    if (p.size() == 1) {
        // Silent accept.
        return;
    }
    // Only channel operators may change modes.
    if (!ch->isOperator(fd)) {
        srv.sendToClient(fd, ERR::chanoprivsneeded(srv.serverName(), c->nick(), chan));
        return;
    }
    // Parse flag string.
    std::string flags = p[1];
    bool add = true;
    size_t argi = 2;
    for (size_t i = 0; i < flags.size(); ++i) {
        char f = flags[i];
        if (f == '+') { add = true; continue; }
        if (f == '-') { add = false; continue; }
        switch (f) {
            case 'i':
                ch->setInviteOnly(add);
                break;
            case 't':
                ch->setTopicOpOnly(add);
                break;
            case 'k':
                if (add) {
                    if (argi < p.size()) ch->setKey(p[argi++]);
                } else ch->clearKey();
                break;
            case 'o':
                if (argi < p.size()) {
                    Client *who = srv.getClientByNick(p[argi++]);
                    if (who) {
                        if (add) ch->addOperator(who->fd());
                        else ch->removeOperator(who->fd());
                    }
                }
                break;
            case 'l':
                if (add) {
                    if (argi < p.size()) {
                        int lim = std::atoi(p[argi++].c_str());
                        if (lim > 0) ch->setLimit((size_t)lim);
                    }
                } else ch->clearLimit();
                break;
            default: break;
        }
    }
    // No verbose response needed for the subject scope.
}

// TOPIC <#chan> [:text]
void CMD::TOPIC(Server &srv, int fd, const std::vector<std::string> &p) {
    Client *c = srv.getClient(fd);
    if (!c || !c->registered()) return;
    if (p.size() < 1) {
        srv.sendToClient(fd, ERR::needmoreparams(srv.serverName(), c->nick(), "TOPIC"));
        return;
    }
    std::string chan = p[0];
    if (chan.empty() || chan[0] != '#') chan = "#" + chan;
    Channel *ch = srv.findChannel(toLower(chan));
    if (!ch) {
        srv.sendToClient(fd, ERR::nosuchchannel(srv.serverName(), c->nick(), chan));
        return;
    }
    if (p.size() == 1) {
        // View
        if (ch->topic().empty()) srv.sendToClient(fd, RPL::notopic(srv.serverName(), c->nick(), chan));
        else srv.sendToClient(fd, RPL::topic(srv.serverName(), c->nick(), chan, ch->topic()));
        return;
    }
    // Modify
    if (ch->topicOpOnly() && !ch->isOperator(fd)) {
        srv.sendToClient(fd, ERR::chanoprivsneeded(srv.serverName(), c->nick(), chan));
        return;
    }
    ch->setTopic(p[1]);
    std::string line = prefixFor(srv, c) + "TOPIC " + chan + " :" + p[1] + "\r\n";
    srv.sendToChannel(chan, fd, line);
    srv.sendToClient(fd, line);
}

// INVITE <nick> <#chan>
void CMD::INVITE(Server &srv, int fd, const std::vector<std::string> &p) {
    Client *c = srv.getClient(fd);
    if (!c || !c->registered()) return;
    if (p.size() < 2) {
        srv.sendToClient(fd, ERR::needmoreparams(srv.serverName(), c->nick(), "INVITE"));
        return;
    }
    std::string nick = p[0];
    std::string chan = p[1];
    if (chan.empty() || chan[0] != '#') chan = "#" + chan;
    Channel *ch = srv.findChannel(toLower(chan));
    if (!ch) {
        srv.sendToClient(fd, ERR::nosuchchannel(srv.serverName(), c->nick(), chan));
        return;
    }
    if (!ch->isOperator(fd)) {
        srv.sendToClient(fd, ERR::chanoprivsneeded(srv.serverName(), c->nick(), chan));
        return;
    }
    Client *target = srv.getClientByNick(nick);
    if (!target) {
        srv.sendToClient(fd, ERR::nosuchnick(srv.serverName(), c->nick(), nick));
        return;
    }
    if (ch->isMember(target->fd())) {
        srv.sendToClient(fd, ERR::useronchannel(srv.serverName(), c->nick(), target->nick(), chan));
        return;
    }
    ch->addInvite(target->fd());
    // Notify inviter and invited.
    srv.sendToClient(fd, RPL::inviting(srv.serverName(), c->nick(), target->nick(), chan));
    std::string line = prefixFor(srv, c) + "INVITE " + target->nick() + " :" + chan + "\r\n";
    srv.sendToClient(target->fd(), line);
}

// KICK <#chan> <nick> [:reason]
void CMD::KICK(Server &srv, int fd, const std::vector<std::string> &p) {
    Client *c = srv.getClient(fd);
    if (!c || !c->registered()) return;
    if (p.size() < 2) {
        srv.sendToClient(fd, ERR::needmoreparams(srv.serverName(), c->nick(), "KICK"));
        return;
    }
    std::string chan = p[0];
    if (chan.empty() || chan[0] != '#') chan = "#" + chan;
    Channel *ch = srv.findChannel(toLower(chan));
    if (!ch) {
        srv.sendToClient(fd, ERR::nosuchchannel(srv.serverName(), c->nick(), chan));
        return;
    }
    if (!ch->isOperator(fd)) {
        srv.sendToClient(fd, ERR::chanoprivsneeded(srv.serverName(), c->nick(), chan));
        return;
    }
    Client *target = srv.getClientByNick(p[1]);
    if (!target || !ch->isMember(target->fd())) {
        srv.sendToClient(fd, ERR::nosuchnick(srv.serverName(), c->nick(), p[1]));
        return;
    }
    std::string reason = (p.size() >= 3) ? p[2] : "Kicked";
    std::string line = prefixFor(srv, c) + "KICK " + chan + " " + target->nick() + " :" + reason + "\r\n";
    srv.sendToChannel(chan, fd, line);
    srv.sendToClient(fd, line);
    // Remove and notify target
    ch->removeMember(target->fd());
    srv.removeChannelIfEmpty(chan);
}

// PING :token
void CMD::PING(Server &srv, int fd, const std::vector<std::string> &p) {
    Client *c = srv.getClient(fd);
    if (!c) return;
    std::string token = p.size()? p[0] : "token";
    std::string pong = ":" + srv.serverName() + " PONG " + srv.serverName() + " :" + token + "\r\n";
    srv.sendToClient(fd, pong);
}


// QUIT [#chan[,#chan2...]] | [:reason]
// Note: Non-standard convenience: if the first param looks like a channel name ('#...'),
// treat this as PART from the given channel(s) while keeping the connection open.
void CMD::QUIT(Server &srv, int fd, const std::vector<std::string> &p) {
    Client *c = srv.getClient(fd);
    if (!c) return;
    if (!p.empty() && !p[0].empty() && p[0][0] == '#') {
        // Behave like PART
        PART(srv, fd, p);
        return;
    }
    std::string reason = p.size()? p[0] : "Quit";
    srv.disconnectClient(fd, reason);
}
