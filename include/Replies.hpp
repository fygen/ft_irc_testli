
#ifndef REPLIES_HPP
#define REPLIES_HPP

#include <string>

// Minimal numeric replies + helpers used in the project.
// Not a full RFC, just what's needed for the subject's features.

namespace RPL {
    // 001 Welcome
    std::string welcome(const std::string &server, const std::string &nick);
    // 332 topic
    std::string topic(const std::string &server, const std::string &nick, const std::string &chan, const std::string &topic);
    // 331 no topic
    std::string notopic(const std::string &server, const std::string &nick, const std::string &chan);
    // 353 names
    std::string namreply(const std::string &server, const std::string &nick, const std::string &chan, const std::string &namelist);
    // 366 end of names
    std::string endofnames(const std::string &server, const std::string &nick, const std::string &chan);
    // 341 inviting
    std::string inviting(const std::string &server, const std::string &nick, const std::string &target, const std::string &chan);
}

namespace ERR {
    std::string nosuchnick(const std::string &server, const std::string &nick, const std::string &target);
    std::string nosuchchannel(const std::string &server, const std::string &nick, const std::string &chan);
    std::string notonchannel(const std::string &server, const std::string &nick, const std::string &chan);
    std::string useronchannel(const std::string &server, const std::string &nick, const std::string &user, const std::string &chan);
    std::string chanoprivsneeded(const std::string &server, const std::string &nick, const std::string &chan);
    std::string needmoreparams(const std::string &server, const std::string &nick, const std::string &cmd);
    std::string alreadyreg(const std::string &server, const std::string &nick);
    std::string passmismatch(const std::string &server, const std::string &nick);
    std::string nicknameinuse(const std::string &server, const std::string &nick);
    std::string badchannelkey(const std::string &server, const std::string &nick, const std::string &chan);
    std::string inviteonlychan(const std::string &server, const std::string &nick, const std::string &chan);
    std::string channelisfull(const std::string &server, const std::string &nick, const std::string &chan);
}

#endif
