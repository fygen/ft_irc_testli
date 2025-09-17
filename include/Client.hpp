
#ifndef CLIENT_HPP
#define CLIENT_HPP

// Line-by-line-ish comments for clarity (kept concise to avoid bloat).
// This header declares the Client class which represents a single connected user.

#include <string>
#include <set>

class Client {
    // Each client is identified by its socket file descriptor (fd).
    int         _fd;
    // Buffered incoming data until we reach a full IRC line (\r\n or \n).
    std::string _inbuf;
    // Outgoing data to write when POLLOUT is ready.
    std::string _outbuf;
    // Registration/identity fields.
    std::string _nick;
    std::string _user;
    std::string _realname;
    bool        _passOk;      // true only after PASS <password> matches
    bool        _registered;  // set true after PASS+NICK+USER succeeds
public:
    // Simple constructor takes the accepted socket descriptor.
    Client(int fd);
    // Accessors.
    int fd() const { return _fd; }
    const std::string &inbuf() const { return _inbuf; }
    const std::string &outbuf() const { return _outbuf; }
    const std::string &nick() const { return _nick; }
    const std::string &user() const { return _user; }
    const std::string &realname() const { return _realname; }
    bool passOk() const { return _passOk; }
    bool registered() const { return _registered; }
    // Mutators.
    void appendIn(const std::string &s) { _inbuf += s; }
    void consumeIn(size_t n) { _inbuf.erase(0, n); }
    void enqueueOut(const std::string &s) { _outbuf += s; }
    void consumeOut(size_t n) { _outbuf.erase(0, n); }
    void setNick(const std::string &n) { _nick = n; }
    void setUser(const std::string &u) { _user = u; }
    void setReal(const std::string &r) { _realname = r; }
    void setPassOk(bool v) { _passOk = v; }
    void setRegistered(bool v) { _registered = v; }
};

#endif
