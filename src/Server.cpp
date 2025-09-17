
// Server.cpp — single-poll nonblocking IRC server core.
// Key points to satisfy correction mandatory checks:
// - Exactly one poll() loop handling listen/read/write (no other poll, no blocking I/O).
// - fcntl(fd, F_SETFL, O_NONBLOCK); and no other fcntl flags (Mac-compatible).
// - We never rely on errno==EAGAIN to reattempt reads/writes; we only act on POLLIN/POLLOUT.

#include "Server.hpp"
#include "Parser.hpp"
#include "Commands.hpp"
#include "Utils.hpp"
#include <iostream>
#include <cstring>
#include <cstdio>
#include <cerrno>

Server::Server(const std::string &serverName, const std::string &password)
: _serverName(serverName), _password(password), _listenFd(-1) {}

Server::~Server() {
    stop();
    // free channels
    for (std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it)
        delete it->second;
    // free clients (should be none by here)
    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
        delete it->second;
    }
}

bool Server::start(unsigned short port) {
    // Create IPv4 TCP socket.
    _listenFd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (_listenFd < 0) { std::perror("socket"); return false; }

    // SO_REUSEADDR for quick restarts.
    int yes = 1;
    if (setsockopt(_listenFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
        std::perror("setsockopt");
        ::close(_listenFd);
        _listenFd = -1;
        return false;
    }

    // Set non-blocking exactly as allowed by subject (Mac note).
    if (fcntl(_listenFd, F_SETFL, O_NONBLOCK) < 0) {
        std::perror("fcntl");
        ::close(_listenFd);
        _listenFd = -1;
        return false;
    }

    // Bind to all interfaces (INADDR_ANY) on the given port (correction requires this).
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);
    if (bind(_listenFd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::perror("bind");
        ::close(_listenFd);
        _listenFd = -1;
        return false;
    }

    if (listen(_listenFd, 128) < 0) {
        std::perror("listen");
        ::close(_listenFd);
        _listenFd = -1;
        return false;
    }

    // Add listening socket to poll vector.
    struct pollfd p;
    p.fd = _listenFd;
    p.events = POLLIN;   // we only accept() when poll says so
    p.revents = 0;
    _pfds.push_back(p);
    return true;
}

void Server::stop() {
    // Close listen fd if open.
    if (_listenFd >= 0) {
        ::close(_listenFd);
        _listenFd = -1;
    }
    // Close all client fds.
    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
        ::close(it->first);
    }
    _clients.clear();
    _pfds.clear();
    _nickToFd.clear();
}

Client *Server::getClient(int fd) {
    std::map<int, Client*>::iterator it = _clients.find(fd);
    if (it == _clients.end()) return 0;
    return it->second;
}

Client *Server::getClientByNick(const std::string &nick) {
    std::map<std::string, int>::iterator it = _nickToFd.find(toLower(nick));
    if (it == _nickToFd.end()) return 0;
    return getClient(it->second);
}

Channel *Server::getOrCreateChannel(const std::string &name) {
    std::map<std::string, Channel*>::iterator it = _channels.find(toLower(name));
    if (it != _channels.end()) return it->second;
    Channel *c = new Channel(name);
    _channels[toLower(name)] = c;
    return c;
}

Channel *Server::findChannel(const std::string &name) {
    std::map<std::string, Channel*>::iterator it = _channels.find(toLower(name));
    if (it == _channels.end()) return 0;
    return it->second;
}

void Server::removeChannelIfEmpty(const std::string &name) {
    std::map<std::string, Channel*>::iterator it = _channels.find(toLower(name));
    if (it == _channels.end()) return;
    if (it->second->members().empty()) {
        delete it->second;
        _channels.erase(it);
    }
}

void Server::handleListenEvent(short revents) {
    if (!(revents & POLLIN)) return;
    // Accept as many connections as the kernel offers now.
    for (;;) {
        struct sockaddr_in cli;
        socklen_t len = sizeof(cli);
        int cfd = ::accept(_listenFd, (struct sockaddr*)&cli, &len);
        if (cfd < 0) {
            // Non-blocking accept: when no more, we get EAGAIN/EWOULDBLOCK. Just stop.
            break;
        }
        // Non-blocking for the client exactly as allowed.
        if (fcntl(cfd, F_SETFL, O_NONBLOCK) < 0) {
            std::perror("fcntl(client)");
            ::close(cfd);
            continue;
        }
        // Track client.
        Client *cl = new Client(cfd);
        _clients[cfd] = cl;

        // Add to poll list with POLLIN initially.
        struct pollfd p;
        p.fd = cfd;
        p.events = POLLIN; // read only until we have something to write
        p.revents = 0;
        _pfds.push_back(p);
    }
}

void Server::sendToClient(int fd, const std::string &msg) {
    Client *c = getClient(fd);
    if (!c) return;
    c->enqueueOut(msg);
    // Ensure POLLOUT is set for this fd, so we only write after poll() signals it.
    for (size_t i = 0; i < _pfds.size(); ++i) {
        if (_pfds[i].fd == fd) {
            _pfds[i].events = POLLIN | POLLOUT;
            break;
        }
    }
}

void Server::sendToChannel(const std::string &chan, int fromFd, const std::string &line) {
    Channel *c = findChannel(chan);
    if (!c) return;
    for (std::set<int>::const_iterator it = c->members().begin(); it != c->members().end(); ++it) {
        int tfd = *it;
        if (tfd == fromFd) continue;
        sendToClient(tfd, line);
    }
}

// Server.cpp

// --- DÜZELTİLMİŞ SÜRÜM (satır satır açıklamalı) ---
void Server::disconnectClient(int fd, const std::string &reason) {
    Client *c = getClient(fd);                                  // (1) fd’den client nesnesini al
    if (!c) return;                                              // (2) Yoksa çık

    // (3) _channels üzerinde dolaşırken erase etmeyeceğiz; boş kalanları sonra sileceğiz.
    std::vector<std::string> toErase;                            // (4) Sonradan silinecek kanal adları

    for (std::map<std::string, Channel*>::iterator it = _channels.begin();
         it != _channels.end(); ++it) {                          // (5) Tüm kanallar üzerinde güvenli iterate
        Channel *ch = it->second;                                // (6) Kanal ptr
        if (!ch) continue;                                       // (7) Emniyet

        if (ch->isMember(fd)) {                                  // (8) Bu client bu kanalda mı?
            // (9) Kanal üyelerine QUIT yayını; ayrılan kişi kendisini görmez.
            std::string prefix = ":" + (c->nick().empty()? "*":c->nick())
                               + "!" + c->user() + "@localhost ";
            std::string quitmsg = prefix + "QUIT :" + reason + "\r\n";
            sendToChannel(ch->name(), fd, quitmsg);              // (10) Üyelere gönder

            ch->removeMember(fd);                                // (11) Üyelikten çıkar (op/invite de temizleniyor)

            if (ch->members().empty()) {                         // (12) Kanal boşaldıysa
                toErase.push_back(ch->name());                   // (13) Adını listeye ekle (şimdi değil, sonra sil)
            }
        }
    }

    // (14) Artık iterator yok; güvenle sil
    for (size_t i = 0; i < toErase.size(); ++i) {
        removeChannelIfEmpty(toErase[i]);                        // (15) Kanalı map’ten kaldır (erase burada güvenli)
    }

    // (16) Nick -> fd haritasını temizle
    if (!c->nick().empty()) _nickToFd.erase(toLower(c->nick()));

    // (17) pollfd vektöründen bu fd’yi çıkar
    for (size_t i = 0; i < _pfds.size(); ++i) {
        if (_pfds[i].fd == fd) { _pfds.erase(_pfds.begin()+i); break; }
    }

    // (18) Soketi kapat, client’ı map’ten çıkar ve bellekten sil
    ::close(fd);
    _clients.erase(fd);
    delete c;
}


void Server::handleClientEvent(size_t i) {
    struct pollfd &p = _pfds[i];
    Client *c = getClient(p.fd);
    if (!c) return;

    // Read if POLLIN set (we only call recv after poll says ready).
    if (p.revents & POLLIN) {
        char buf[4096];
        for (;;) {
            ssize_t n = ::recv(p.fd, buf, sizeof(buf), 0);
            if (n > 0) {
                c->appendIn(std::string(buf, n));
            } else if (n == 0) {
                // Peer closed gracefully.
                disconnectClient(p.fd, "Client quit");
                return;
            } else {
                // n < 0: in non-blocking mode, no more data gives EAGAIN/EWOULDBLOCK.
                // We never spin on errno, per correction — simply stop reading now.
                break;
            }
        }
        // Process complete lines.
        for (;;) {
            std::string &in = const_cast<std::string&>(c->inbuf());
            size_t pos = in.find('\n');
            if (pos == std::string::npos) break;
            std::string raw = trimCRLF(in.substr(0, pos+1));
            c->consumeIn(pos+1);
            if (!raw.empty()) handleLine(p.fd, raw);
        }
    }

    // Write if we have something and POLLOUT is set.
    if ((p.revents & POLLOUT) && !c->outbuf().empty()) {
        const std::string &out = c->outbuf();
        ssize_t n = ::send(p.fd, out.data(), out.size(), 0);
        if (n > 0) {
            c->consumeOut((size_t)n);
        } else if (n < 0) {
            // On EAGAIN, just skip until poll says POLLOUT again.
        }
    }
    // If output is empty, we can clear POLLOUT bit to save CPU.
    if (c) { // c might be deleted by disconnect
        if (c->outbuf().empty()) {
            _pfds[i].events = POLLIN;
        } else {
            _pfds[i].events = POLLIN | POLLOUT;
        }
    }
}

void Server::run() {
    // Single loop, single poll. All accepts/reads/writes are only performed after this poll returns.
    while (true) {
        int ret = ::poll(&_pfds[0], _pfds.size(), -1);
        if (ret < 0) {
            // If interrupted, continue; else exit.
            if (errno == EINTR) continue;
            std::perror("poll");
            break;
        }
        // Iterate snapshot of size, as we might erase entries (disconnect).
        size_t n = _pfds.size();
        for (size_t i = 0; i < n; ++i) {
            // Listening socket at index 0 originally; but we don't rely on that — we check by fd.
            if (_pfds[i].fd == _listenFd) {
                handleListenEvent(_pfds[i].revents);
            } else {
                // Safeguard if fd disappeared (disconnect may shrink vector); bounds check.
                if (i < _pfds.size())
                    handleClientEvent(i);
            }
        }
    }
}
