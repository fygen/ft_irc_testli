
#include "Replies.hpp"

static std::string pfx(const std::string &server) {
    return ":" + server + " ";
}

namespace RPL {
std::string welcome(const std::string &server, const std::string &nick) {
    return pfx(server) + "001 " + nick + " :Welcome to ft_irc, " + nick + "\r\n";
}
std::string topic(const std::string &server, const std::string &nick, const std::string &chan, const std::string &topic) {
    return pfx(server) + "332 " + nick + " " + chan + " :" + topic + "\r\n";
}
std::string notopic(const std::string &server, const std::string &nick, const std::string &chan) {
    return pfx(server) + "331 " + nick + " " + chan + " :No topic is set\r\n";
}
std::string namreply(const std::string &server, const std::string &nick, const std::string &chan, const std::string &namelist) {
    return pfx(server) + "353 " + nick + " = " + chan + " :" + namelist + "\r\n";
}
std::string endofnames(const std::string &server, const std::string &nick, const std::string &chan) {
    return pfx(server) + "366 " + nick + " " + chan + " :End of /NAMES list.\r\n";
}
std::string inviting(const std::string &server, const std::string &nick, const std::string &target, const std::string &chan) {
    return pfx(server) + "341 " + nick + " " + target + " " + chan + "\r\n";
}
}

namespace ERR {
std::string nosuchnick(const std::string &server, const std::string &nick, const std::string &target) {
    return pfx(server) + "401 " + nick + " " + target + " :No such nick\r\n";
}
std::string nosuchchannel(const std::string &server, const std::string &nick, const std::string &chan) {
    return pfx(server) + "403 " + nick + " " + chan + " :No such channel\r\n";
}
std::string notonchannel(const std::string &server, const std::string &nick, const std::string &chan) {
    return pfx(server) + "442 " + nick + " " + chan + " :You're not on that channel\r\n";
}
std::string useronchannel(const std::string &server, const std::string &nick, const std::string &user, const std::string &chan) {
    return pfx(server) + "443 " + nick + " " + user + " " + chan + " :is already on channel\r\n";
}
std::string chanoprivsneeded(const std::string &server, const std::string &nick, const std::string &chan) {
    return pfx(server) + "482 " + nick + " " + chan + " :You're not channel operator\r\n";
}
std::string needmoreparams(const std::string &server, const std::string &nick, const std::string &cmd) {
    return pfx(server) + "461 " + nick + " " + cmd + " :Not enough parameters\r\n";
}
std::string alreadyreg(const std::string &server, const std::string &nick) {
    return pfx(server) + "462 " + nick + " :You may not reregister\r\n";
}
std::string passmismatch(const std::string &server, const std::string &nick) {
    return pfx(server) + "464 " + nick + " :Password incorrect\r\n";
}
std::string nicknameinuse(const std::string &server, const std::string &nick) {
    return pfx(server) + "433 * " + nick + " :Nickname is already in use\r\n";
}
std::string badchannelkey(const std::string &server, const std::string &nick, const std::string &chan) {
    return pfx(server) + "475 " + nick + " " + chan + " :Cannot join channel (+k)\r\n";
}
std::string inviteonlychan(const std::string &server, const std::string &nick, const std::string &chan) {
    return pfx(server) + "473 " + nick + " " + chan + " :Cannot join channel (+i)\r\n";
}
std::string channelisfull(const std::string &server, const std::string &nick, const std::string &chan) {
    return pfx(server) + "471 " + nick + " " + chan + " :Cannot join channel (+l)\r\n";
}
}
