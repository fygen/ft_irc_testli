
#ifndef PARSER_HPP
#define PARSER_HPP

// A tiny helper to parse IRC lines into: command + parameters (vector).

#include <string>
#include <vector>

struct ParsedLine {
    std::string command;
    std::vector<std::string> params;
};

// Parse one IRC line (without the trailing CRLF).
// Handles a single trailing parameter introduced by ':' per RFC-style.
ParsedLine parseIrcLine(const std::string &line);

#endif
