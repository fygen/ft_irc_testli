
#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <vector>

std::string toLower(const std::string &s);
std::string trimCRLF(const std::string &s);
bool isValidNick(const std::string &nick);
std::vector<std::string> split(const std::string &s, char delim);
std::string itostr(int v);

#endif
