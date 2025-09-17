
#include "Server.hpp"
#include "Parser.hpp"
#include "Commands.hpp"
#include "Utils.hpp"
#include <iostream>

void Server::handleLine(int fd, const std::string &line) {
    // Parse and dispatch a single IRC command line.
    Client *c = getClient(fd);
    if (!c) 
		return;
    ParsedLine pl = parseIrcLine(line);
    std::string cmd = toLower(pl.command);

    if (cmd == "pass") CMD::PASS(*this, fd, pl.params);
    else if (cmd == "nick") CMD::NICK(*this, fd, pl.params);
    else if (cmd == "user") CMD::USER(*this, fd, pl.params);
    else if (cmd == "join") CMD::JOIN(*this, fd, pl.params);
    else if (cmd == "part") CMD::PART(*this, fd, pl.params);
    else if (cmd == "privmsg") CMD::PRIVMSG(*this, fd, pl.params);
    else if (cmd == "mode") CMD::MODE(*this, fd, pl.params);
    else if (cmd == "topic") CMD::TOPIC(*this, fd, pl.params);
    else if (cmd == "invite") CMD::INVITE(*this, fd, pl.params);
    else if (cmd == "kick") CMD::KICK(*this, fd, pl.params);
    else if (cmd == "ping") CMD::PING(*this, fd, pl.params);
    else if (cmd == "quit") CMD::QUIT(*this, fd, pl.params);
    else {
        // Silently ignore unknown commands to keep the server simple/human-like.
        (void)0;
    }
}
