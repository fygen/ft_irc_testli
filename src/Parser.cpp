
#include "Parser.hpp"
#include "Utils.hpp"

ParsedLine parseIrcLine(const std::string &line) {
    ParsedLine out;
    // Basic split by spaces except when a param starts with ':' -> trailing
    std::vector<std::string> toks = split(line, ' ');
    if (toks.empty()) return out;
    out.command = toks[0];
    // Gather params; if we encounter a token starting with ':', rest of line is one param
    bool trailing = false;
    std::string trail;
    for (size_t i = 1; i < toks.size(); ++i) {
        if (!trailing && !toks[i].empty() && toks[i][0] == ':') {
            trailing = true;
            // strip leading ':'
            trail = toks[i].substr(1);
        } else if (trailing) {
            if (!trail.empty()) trail += " ";
            trail += toks[i];
        } else {
            out.params.push_back(toks[i]);
        }
    }
    if (trailing) out.params.push_back(trail);
    return out;
}
