
#ifndef COMMANDS_HPP
#define COMMANDS_HPP

#include <string>
#include <vector>

class Server;
class Client;
class Channel;

// Each handler validates parameters, checks privileges and updates server state.
// Only features required by the subject are implemented.

namespace CMD {
    void PASS(Server &srv, int fd, const std::vector<std::string> &p);
    void NICK(Server &srv, int fd, const std::vector<std::string> &p);
    void USER(Server &srv, int fd, const std::vector<std::string> &p);
    void JOIN(Server &srv, int fd, const std::vector<std::string> &p);
    void PART(Server &srv, int fd, const std::vector<std::string> &p);
    void PRIVMSG(Server &srv, int fd, const std::vector<std::string> &p);
    void MODE(Server &srv, int fd, const std::vector<std::string> &p);
    void TOPIC(Server &srv, int fd, const std::vector<std::string> &p);
    void INVITE(Server &srv, int fd, const std::vector<std::string> &p);
    void KICK(Server &srv, int fd, const std::vector<std::string> &p);
    void PING(Server &srv, int fd, const std::vector<std::string> &p);
    void QUIT(Server &srv, int fd, const std::vector<std::string> &p);
}

#endif
