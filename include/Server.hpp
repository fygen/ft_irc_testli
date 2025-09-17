
#ifndef SERVER_HPP
#define SERVER_HPP

// The Server orchestrates sockets, poll(), clients, channels and command handling.

#include <string>
#include <map>
#include <vector>
#include <set>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>

#include "Client.hpp"
#include "Channel.hpp"

class Server {
    std::string _serverName;        // used in replies prefix
    std::string _password;          // PASS <password>
    int         _listenFd;          // listening socket
    std::vector<struct pollfd> _pfds; // single poll() vector (only one poll used globally)
    std::map<int, Client*> _clients;     // by fd
    std::map<std::string, int> _nickToFd; // nick to fd
    std::map<std::string, Channel*> _channels; // by channel name
public:
    Server(const std::string &serverName, const std::string &password);
    ~Server();

    bool start(unsigned short port); // create/bind/listen
    void run();                      // main loop with a single poll()
    void stop();                     // cleanup sockets

    // Helpers for client management
    void handleListenEvent(short revents);
    void handleClientEvent(size_t i); // i is index in _pfds
    void disconnectClient(int fd, const std::string &reason);

    // Sending helpers
    void sendToClient(int fd, const std::string &msg); // enqueue + enable POLLOUT
    void sendToChannel(const std::string &chan, int fromFd, const std::string &line);

    // State access
    Client *getClient(int fd);
    Client *getClientByNick(const std::string &nick);
    Channel *getOrCreateChannel(const std::string &name);
    Channel *findChannel(const std::string &name);
    void removeChannelIfEmpty(const std::string &name);

    // Command dispatcher
    void handleLine(int fd, const std::string &line);
    const std::string &serverName() const { return _serverName; }
    const std::string &password() const { return _password; }
    std::map<int, Client*> &clients() { return _clients; }
    std::map<std::string, Channel*> &channels() { return _channels; }
    std::map<std::string, int> &nickToFd() { return _nickToFd; }
};

#endif
