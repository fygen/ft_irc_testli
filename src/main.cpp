
// main.cpp — entry point; minimal argument parsing and server start.
// We keep it small and readable by design.

#include <iostream>
#include <sstream>
#include <cstdlib>
#include "Server.hpp"

static bool parsePort(const char *s, unsigned short &out) {
    // Convert string to number and ensure 1..65535
    long v = 0;
    std::istringstream iss(s);
    iss >> v;
    if (!iss.fail() && v > 0 && v <= 65535) {
        out = static_cast<unsigned short>(v);
        return true;
    }
    return false;
}

int main(int argc, char **argv) {
    // Require: ./ircserv <port> <password>
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <port> <password>\n";
        return 1;
    }
    unsigned short port = 0;
    if (!parsePort(argv[1], port)) {
        std::cerr << "Invalid port.\n";
        return 1;
    }
    std::string password = argv[2];
    if (password.empty()) {
        std::cerr << "Password must be non-empty.\n";
        return 1;
    }

    // Initialize server with a human-readable name.
    Server srv("ft_irc.min", password);
    if (!srv.start(port)) {
        std::cerr << "Failed to start server.\n";
        return 1;
    }
    // Enter the single-poll() loop.
    srv.run();
    return 0;
}
