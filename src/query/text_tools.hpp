#pragma once
#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>
#include <vector>

namespace db {

inline std::string trim(std::string s) {
    auto not_space = [](unsigned char c) { return !std::isspace(c); };
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), not_space));
    s.erase(std::find_if(s.rbegin(), s.rend(), not_space).base(), s.end());
    return s;
}

inline std::string upper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return s;
}

inline bool starts_ci(const std::string& text, const std::string& prefix) {
    if (text.size() < prefix.size()) return false;
    return upper(text.substr(0, prefix.size())) == upper(prefix);
}

inline std::string strip_quotes(std::string value) {
    value = trim(std::move(value));
    if (value.size() >= 2) {
        char a = value.front();
        char b = value.back();
        if ((a == '\'' && b == '\'') || (a == '"' && b == '"')) {
            value = value.substr(1, value.size() - 2);
        }
    }
    std::string out;
    bool esc = false;
    for (char c : value) {
        if (esc) { out.push_back(c); esc = false; }
        else if (c == '\\') esc = true;
        else out.push_back(c);
    }
    if (esc) out.push_back('\\');
    return out;
}

inline bool is_word_char(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

std::vector<std::string> split_statements(const std::string& script);
std::vector<std::string> split_top_level(const std::string& text, char delimiter);
std::vector<std::string> tokenize_words(const std::string& text);
std::string json_escape(const std::string& value);

} // namespace db
