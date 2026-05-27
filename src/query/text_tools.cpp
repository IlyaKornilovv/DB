#include "text_tools.hpp"

namespace db {

std::vector<std::string> split_statements(const std::string& script) {
    std::vector<std::string> out;
    std::string cur;
    char quote = 0;
    int bracket_depth = 0;
    bool esc = false;
    for (char ch : script) {
        if (esc) { cur.push_back(ch); esc = false; continue; }
        if (quote && ch == '\\') { cur.push_back(ch); esc = true; continue; }
        if (quote) {
            cur.push_back(ch);
            if (ch == quote) quote = 0;
            continue;
        }
        if (ch == '\'' || ch == '"') { quote = ch; cur.push_back(ch); continue; }
        if (ch == '(' || ch == '[') ++bracket_depth;
        if ((ch == ')' || ch == ']') && bracket_depth > 0) --bracket_depth;
        if (ch == ';' && bracket_depth == 0) {
            auto item = trim(cur);
            if (!item.empty()) out.push_back(item);
            cur.clear();
        } else {
            cur.push_back(ch);
        }
    }
    auto item = trim(cur);
    if (!item.empty()) out.push_back(item);
    return out;
}

std::vector<std::string> split_top_level(const std::string& text, char delimiter) {
    std::vector<std::string> out;
    std::string cur;
    char quote = 0;
    int round = 0, square = 0;
    bool esc = false;
    for (char ch : text) {
        if (esc) { cur.push_back(ch); esc = false; continue; }
        if (quote && ch == '\\') { cur.push_back(ch); esc = true; continue; }
        if (quote) {
            cur.push_back(ch);
            if (ch == quote) quote = 0;
            continue;
        }
        if (ch == '\'' || ch == '"') { quote = ch; cur.push_back(ch); continue; }
        if (ch == '(') ++round;
        else if (ch == ')' && round > 0) --round;
        else if (ch == '[') ++square;
        else if (ch == ']' && square > 0) --square;
        if (ch == delimiter && round == 0 && square == 0) {
            out.push_back(trim(cur));
            cur.clear();
        } else {
            cur.push_back(ch);
        }
    }
    if (!cur.empty() || !text.empty()) out.push_back(trim(cur));
    return out;
}

std::vector<std::string> tokenize_words(const std::string& text) {
    std::vector<std::string> tokens;
    std::string cur;
    char quote = 0;
    bool esc = false;
    for (char ch : text) {
        if (esc) { cur.push_back(ch); esc = false; continue; }
        if (quote && ch == '\\') { cur.push_back(ch); esc = true; continue; }
        if (quote) {
            cur.push_back(ch);
            if (ch == quote) { tokens.push_back(cur); cur.clear(); quote = 0; }
            continue;
        }
        if (ch == '\'' || ch == '"') {
            if (!cur.empty()) { tokens.push_back(cur); cur.clear(); }
            quote = ch;
            cur.push_back(ch);
        } else if (std::isspace(static_cast<unsigned char>(ch))) {
            if (!cur.empty()) { tokens.push_back(cur); cur.clear(); }
        } else {
            cur.push_back(ch);
        }
    }
    if (!cur.empty()) tokens.push_back(cur);
    return tokens;
}

std::string json_escape(const std::string& value) {
    std::string out;
    for (char ch : value) {
        switch (ch) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (static_cast<unsigned char>(ch) < 0x20) out += ' ';
                else out += ch;
        }
    }
    return out;
}

} // namespace db
