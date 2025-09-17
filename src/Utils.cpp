
#include "Utils.hpp"
#include <cctype>
#include <sstream>

std::string toLower(const std::string &s) {
    std::string r(s);
    for (size_t i = 0; i < r.size(); ++i) r[i] = std::tolower(r[i]);
    return r;
}

std::string trimCRLF(const std::string &s) {
    // Strip trailing CR and/or LF.
    size_t end = s.size();
    while (end > 0 && (s[end-1] == '\n' || s[end-1] == '\r')) --end;
    return s.substr(0, end);
}

bool isValidNick(const std::string &nick) {
    // Very permissive, but avoid spaces and control chars.
    if (nick.empty()) return false;
    for (size_t i = 0; i < nick.size(); ++i) {
        unsigned char c = nick[i];
        if (c <= 32 || c == 127 || c == ',' || c == '*') return false;
    }
    return true;
}

std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> out;
    std::string cur;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == delim) {
            if (!cur.empty()) out.push_back(cur);
            cur.clear();
        } else cur += s[i];
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}

std::string itostr(int v) {
    std::ostringstream oss;
    oss << v;
    return oss.str();
}
