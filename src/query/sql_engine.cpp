#include "sql_engine.hpp"
#include "text_tools.hpp"

#include <cmath>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace db {
namespace {

std::string ok_json(const std::string& type, const std::string& message, int affected = -1) {
    std::ostringstream out;
    out << "{\"type\":\"" << type << "\"";
    if (type == "ddl") out << ",\"success\":true";
    if (affected >= 0) out << ",\"affected_rows\":" << affected;
    out << ",\"message\":\"" << json_escape(message) << "\"}";
    return out.str();
}

std::string error_json(const std::string& message) {
    return "{\"type\":\"error\",\"message\":\"" + json_escape(message) + "\"}";
}

std::string ident_regex() { return "([A-Za-z_][A-Za-z0-9_]*)"; }

std::string extract_name_after(const std::string& sql, const std::string& keyword_regex) {
    std::regex re(keyword_regex + "\\s+" + ident_regex(), std::regex_constants::icase);
    std::smatch m;
    if (!std::regex_search(sql, m, re)) throw std::runtime_error("Expected identifier");
    return m[1].str();
}

int column_index(const TableData& table, const std::string& name) {
    for (size_t i = 0; i < table.columns.size(); ++i) {
        if (upper(table.columns[i].name) == upper(name)) return static_cast<int>(i);
    }
    throw std::runtime_error("Unknown column: " + name);
}

bool is_number(const std::string& value) {
    if (value.empty()) return false;
    char* end = nullptr;
    std::strtod(value.c_str(), &end);
    return end && *end == 0;
}

bool truthy_bool(const std::string& value) {
    auto u = upper(value);
    return u == "TRUE" || u == "1" || u == "YES";
}

bool validate_kind(const ColumnSpec& col, const std::string& value) {
    if (value.empty() || upper(value) == "NULL") return !col.auto_increment;
    switch (col.kind) {
        case FieldKind::Int: {
            std::regex r("[-+]?[0-9]+");
            return std::regex_match(value, r);
        }
        case FieldKind::Float:
            return is_number(value);
        case FieldKind::Bool: {
            auto u = upper(value);
            return u == "TRUE" || u == "FALSE" || u == "1" || u == "0";
        }
        case FieldKind::Varchar:
            return col.max_len <= 0 || static_cast<int>(value.size()) <= col.max_len;
        case FieldKind::TextArray: {
            auto t = trim(value);
            return t.size() >= 2 && t.front() == '[' && t.back() == ']';
        }
        case FieldKind::Text:
            return true;
    }
    return true;
}

std::string normalize_value(const ColumnSpec& col, std::string value) {
    value = strip_quotes(std::move(value));
    if (col.kind == FieldKind::Bool) return truthy_bool(value) ? "true" : "false";
    return value;
}

std::vector<std::string> extract_parenthesized_groups(const std::string& text) {
    std::vector<std::string> groups;
    char quote = 0;
    bool esc = false;
    int depth = 0;
    std::string cur;
    for (char ch : text) {
        if (esc) { if (depth > 0) cur.push_back(ch); esc = false; continue; }
        if (quote && ch == '\\') { if (depth > 0) cur.push_back(ch); esc = true; continue; }
        if (quote) {
            if (depth > 0) cur.push_back(ch);
            if (ch == quote) quote = 0;
            continue;
        }
        if (ch == '\'' || ch == '"') { if (depth > 0) cur.push_back(ch); quote = ch; continue; }
        if (ch == '(') {
            if (depth++ > 0) cur.push_back(ch);
        } else if (ch == ')') {
            if (--depth == 0) {
                groups.push_back(trim(cur));
                cur.clear();
            } else if (depth > 0) cur.push_back(ch);
            if (depth < 0) depth = 0;
        } else if (depth > 0) {
            cur.push_back(ch);
        }
    }
    return groups;
}

std::string read_after_keyword(const std::string& sql, const std::string& keyword) {
    std::regex re("\\b" + keyword + "\\b", std::regex_constants::icase);
    std::smatch m;
    if (!std::regex_search(sql, m, re)) return "";
    return trim(sql.substr(static_cast<size_t>(m.position() + m.length())));
}

size_t find_keyword_top(const std::string& text, const std::string& word) {
    std::string u = upper(text);
    std::string w = upper(word);
    char quote = 0;
    int depth = 0;
    for (size_t i = 0; i + w.size() <= text.size(); ++i) {
        char ch = text[i];
        if (quote) { if (ch == quote) quote = 0; continue; }
        if (ch == '\'' || ch == '"') { quote = ch; continue; }
        if (ch == '(' || ch == '[') ++depth;
        else if ((ch == ')' || ch == ']') && depth > 0) --depth;
        if (depth == 0 && u.compare(i, w.size(), w) == 0) {
            bool before = i == 0 || !is_word_char(text[i - 1]);
            bool after = i + w.size() == text.size() || !is_word_char(text[i + w.size()]);
            if (before && after) return i;
        }
    }
    return std::string::npos;
}

std::vector<std::pair<std::string, std::string>> split_by_logical(const std::string& condition) {
    std::vector<std::pair<std::string, std::string>> parts;
    std::string rest = trim(condition);
    std::string next_op;
    while (!rest.empty()) {
        size_t and_pos = find_keyword_top(rest, "AND");
        size_t or_pos = find_keyword_top(rest, "OR");
        size_t pos = std::string::npos;
        std::string op;
        if (and_pos != std::string::npos && (or_pos == std::string::npos || and_pos < or_pos)) { pos = and_pos; op = "AND"; }
        else if (or_pos != std::string::npos) { pos = or_pos; op = "OR"; }
        if (pos == std::string::npos) {
            parts.push_back({next_op, trim(rest)});
            break;
        }
        parts.push_back({next_op, trim(rest.substr(0, pos))});
        next_op = op;
        rest = trim(rest.substr(pos + op.size()));
    }
    return parts;
}

bool compare_values(const std::string& left, const std::string& op, const std::string& right) {
    if (is_number(left) && is_number(right)) {
        double a = std::stod(left);
        double b = std::stod(right);
        if (op == "=") return std::fabs(a - b) < 1e-9;
        if (op == "<>") return std::fabs(a - b) >= 1e-9;
        if (op == ">") return a > b;
        if (op == "<") return a < b;
        if (op == ">=") return a >= b;
        if (op == "<=") return a <= b;
    }
    if (op == "=") return left == right;
    if (op == "<>") return left != right;
    if (op == ">") return left > right;
    if (op == "<") return left < right;
    if (op == ">=") return left >= right;
    if (op == "<=") return left <= right;
    return false;
}

bool row_matches(const TableData& table, const std::vector<std::string>& row, const std::string& condition) {
    auto cond = trim(condition);
    if (cond.empty()) return true;
    if (starts_ci(cond, "NOT ")) return !row_matches(table, row, cond.substr(4));
    auto pieces = split_by_logical(cond);
    bool result = true;
    bool first = true;
    for (const auto& [logical, expr_raw] : pieces) {
        std::string expr = trim(expr_raw);
        std::string op;
        for (const auto& candidate : {std::string(">="), std::string("<="), std::string("<>"), std::string("="), std::string(">"), std::string("<")}) {
            auto pos = expr.find(candidate);
            if (pos != std::string::npos) { op = candidate; break; }
        }
        if (op.empty()) throw std::runtime_error("Unsupported WHERE condition: " + expr);
        auto pos = expr.find(op);
        std::string col = trim(expr.substr(0, pos));
        std::string value = strip_quotes(trim(expr.substr(pos + op.size())));
        int idx = column_index(table, col);
        bool current = compare_values(row.at(static_cast<size_t>(idx)), op, value);
        if (first) result = current;
        else if (logical == "AND") result = result && current;
        else if (logical == "OR") result = result || current;
        first = false;
    }
    return result;
}

std::string select_json(const std::vector<std::string>& columns, const std::vector<std::vector<std::string>>& rows) {
    std::ostringstream out;
    out << "{\"type\":\"select\",\"columns\":[";
    for (size_t i = 0; i < columns.size(); ++i) {
        if (i) out << ',';
        out << "\"" << json_escape(columns[i]) << "\"";
    }
    out << "],\"rows\":[";
    for (size_t r = 0; r < rows.size(); ++r) {
        if (r) out << ',';
        out << '[';
        for (size_t c = 0; c < rows[r].size(); ++c) {
            if (c) out << ',';
            out << "\"" << json_escape(rows[r][c]) << "\"";
        }
        out << ']';
    }
    out << "],\"affected_rows\":" << rows.size() << "}";
    return out.str();
}

} // namespace

SqlEngine::SqlEngine(std::filesystem::path data_root) : catalog_(std::move(data_root)) {
    catalog_.load();
}

std::string SqlEngine::execute_batch(const std::string& sql) {
    std::lock_guard<std::mutex> lock(catalog_.mutex());
    try {
        std::string last = ok_json("ddl", "Empty query");
        for (const auto& statement : split_statements(sql)) last = execute_one(statement);
        return last;
    } catch (const std::exception& e) {
        return error_json(e.what());
    }
}

std::string SqlEngine::execute_one(const std::string& sql) {
    auto s = trim(sql);
    if (s.empty()) return ok_json("ddl", "Empty query");
    if (starts_ci(s, "CREATE DATABASE")) return create_database(s);
    if (starts_ci(s, "DROP DATABASE")) return drop_database(s);
    if (starts_ci(s, "USE")) return use_database(s);
    if (starts_ci(s, "CREATE TABLE")) return create_table(s);
    if (starts_ci(s, "DROP TABLE")) return drop_table(s);
    if (starts_ci(s, "INSERT INTO")) return insert_into(s);
    if (starts_ci(s, "SELECT")) return select_from(s);
    if (starts_ci(s, "UPDATE")) return update_rows(s);
    if (starts_ci(s, "DELETE FROM")) return delete_rows(s);
    if (starts_ci(s, "SHOW")) return show_items(s);
    if (starts_ci(s, "CREATE INDEX") || starts_ci(s, "DROP INDEX")) return ok_json("ddl", "Index command accepted");
    throw std::runtime_error("Unsupported SQL: " + s);
}

std::string SqlEngine::create_database(const std::string& sql) {
    auto name = extract_name_after(sql, "CREATE\\s+DATABASE");
    catalog_.create_database(name);
    catalog_.save();
    return ok_json("ddl", "Database " + name + " created");
}

std::string SqlEngine::drop_database(const std::string& sql) {
    auto name = extract_name_after(sql, "DROP\\s+DATABASE");
    catalog_.drop_database(name);
    catalog_.save();
    return ok_json("ddl", "Database " + name + " dropped");
}

std::string SqlEngine::use_database(const std::string& sql) {
    auto tokens = tokenize_words(sql);
    if (tokens.size() < 2) throw std::runtime_error("USE requires database name");
    auto name = tokens[1];
    catalog_.use_database(name);
    catalog_.save();
    return ok_json("ddl", "Using database " + name);
}

std::string SqlEngine::create_table(const std::string& sql) {
    std::regex re("CREATE\\s+TABLE\\s+" + ident_regex() + "\\s*\\((.*)\\)\\s*$", std::regex_constants::icase);
    std::smatch m;
    if (!std::regex_match(sql, m, re)) throw std::runtime_error("Invalid CREATE TABLE syntax");
    std::string table_name = m[1].str();
    auto defs = split_top_level(m[2].str(), ',');
    std::vector<ColumnSpec> columns;
    for (const auto& def : defs) {
        auto words = tokenize_words(def);
        if (words.size() < 2) throw std::runtime_error("Invalid column definition: " + def);
        ColumnSpec c;
        c.name = words[0];
        auto type_text = upper(words[1]);
        std::smatch varchar_match;
        if (std::regex_match(type_text, varchar_match, std::regex("VARCHAR\\((\\d+)\\)"))) {
            c.kind = FieldKind::Varchar;
            c.max_len = std::stoi(varchar_match[1].str());
        } else {
            c.kind = kind_from_text(type_text);
        }
        for (size_t i = 2; i < words.size(); ++i) {
            auto w = upper(words[i]);
            if (w == "AUTO_INCREMENT" || w == "AUTOINCREMENT") c.auto_increment = true;
            if (w == "UNIQUE") c.unique = true;
        }
        columns.push_back(c);
    }
    catalog_.create_table(table_name, std::move(columns));
    catalog_.save();
    return ok_json("ddl", "Table " + table_name + " created");
}

std::string SqlEngine::drop_table(const std::string& sql) {
    auto name = extract_name_after(sql, "DROP\\s+TABLE");
    catalog_.drop_table(name);
    catalog_.save();
    return ok_json("ddl", "Table " + name + " dropped");
}

std::string SqlEngine::insert_into(const std::string& sql) {
    std::regex re("INSERT\\s+INTO\\s+" + ident_regex() + "\\s*(\\([^)]*\\))?\\s+VALUES\\s+(.*)$", std::regex_constants::icase);
    std::smatch m;
    if (!std::regex_match(sql, m, re)) throw std::runtime_error("Invalid INSERT syntax");
    std::string table_name = m[1].str();
    std::string cols_raw = m[2].str();
    std::string values_raw = m[3].str();
    auto& table = catalog_.table(table_name);
    std::vector<int> target_indexes;
    if (!trim(cols_raw).empty()) {
        std::string inner = trim(cols_raw);
        inner = inner.substr(1, inner.size() - 2);
        for (const auto& col : split_top_level(inner, ',')) target_indexes.push_back(column_index(table, col));
    } else {
        for (size_t i = 0; i < table.columns.size(); ++i) target_indexes.push_back(static_cast<int>(i));
    }

    int inserted = 0;
    for (const auto& group : extract_parenthesized_groups(values_raw)) {
        auto values = split_top_level(group, ',');
        if (values.size() != target_indexes.size()) throw std::runtime_error("Column count does not match value count");
        std::vector<std::string> row(table.columns.size(), "");
        for (size_t i = 0; i < values.size(); ++i) {
            int idx = target_indexes[i];
            auto& col = table.columns[static_cast<size_t>(idx)];
            row[static_cast<size_t>(idx)] = normalize_value(col, values[i]);
        }
        for (size_t i = 0; i < table.columns.size(); ++i) {
            auto& col = table.columns[i];
            if (col.auto_increment && (row[i].empty() || upper(row[i]) == "NULL")) row[i] = std::to_string(col.next_value++);
            else if (col.auto_increment && is_number(row[i])) col.next_value = std::max(col.next_value, std::stoll(row[i]) + 1);
            if (!validate_kind(col, row[i])) throw std::runtime_error("Invalid value for column: " + col.name);
            if (col.unique) {
                for (const auto& old_row : table.rows) {
                    if (i < old_row.size() && old_row[i] == row[i]) throw std::runtime_error("UNIQUE constraint failed: " + col.name);
                }
            }
        }
        table.rows.push_back(std::move(row));
        ++inserted;
    }
    catalog_.save();
    return ok_json("dml", std::to_string(inserted) + " row(s) inserted", inserted);
}

std::string SqlEngine::select_from(const std::string& sql) {
    std::regex re("SELECT\\s+(.+)\\s+FROM\\s+" + ident_regex() + "(.*)$", std::regex_constants::icase);
    std::smatch m;
    if (!std::regex_match(sql, m, re)) throw std::runtime_error("Invalid SELECT syntax");
    auto projection = trim(m[1].str());
    auto table_name = m[2].str();
    auto tail = trim(m[3].str());
    std::string where;
    auto where_pos = find_keyword_top(tail, "WHERE");
    if (where_pos != std::string::npos) where = trim(tail.substr(where_pos + 5));
    const auto& table = catalog_.table(table_name);

    std::vector<int> indexes;
    std::vector<std::string> names;
    if (projection == "*") {
        for (size_t i = 0; i < table.columns.size(); ++i) {
            indexes.push_back(static_cast<int>(i));
            names.push_back(table.columns[i].name);
        }
    } else {
        for (const auto& col : split_top_level(projection, ',')) {
            int idx = column_index(table, col);
            indexes.push_back(idx);
            names.push_back(table.columns[static_cast<size_t>(idx)].name);
        }
    }
    std::vector<std::vector<std::string>> rows;
    for (const auto& row : table.rows) {
        if (!row_matches(table, row, where)) continue;
        std::vector<std::string> selected;
        for (int idx : indexes) selected.push_back(row.at(static_cast<size_t>(idx)));
        rows.push_back(std::move(selected));
    }
    return select_json(names, rows);
}

std::string SqlEngine::update_rows(const std::string& sql) {
    std::regex re("UPDATE\\s+" + ident_regex() + "\\s+SET\\s+(.+)$", std::regex_constants::icase);
    std::smatch m;
    if (!std::regex_match(sql, m, re)) throw std::runtime_error("Invalid UPDATE syntax");
    std::string table_name = m[1].str();
    std::string rest = trim(m[2].str());
    std::string where;
    auto where_pos = find_keyword_top(rest, "WHERE");
    if (where_pos != std::string::npos) {
        where = trim(rest.substr(where_pos + 5));
        rest = trim(rest.substr(0, where_pos));
    }
    auto& table = catalog_.table(table_name);
    std::vector<std::pair<int, std::string>> assignments;
    for (const auto& item : split_top_level(rest, ',')) {
        auto eq = item.find('=');
        if (eq == std::string::npos) throw std::runtime_error("Invalid assignment: " + item);
        int idx = column_index(table, trim(item.substr(0, eq)));
        auto val = normalize_value(table.columns[static_cast<size_t>(idx)], item.substr(eq + 1));
        if (!validate_kind(table.columns[static_cast<size_t>(idx)], val)) throw std::runtime_error("Invalid value for column");
        assignments.push_back({idx, val});
    }
    int changed = 0;
    for (auto& row : table.rows) {
        if (!row_matches(table, row, where)) continue;
        for (const auto& [idx, val] : assignments) row[static_cast<size_t>(idx)] = val;
        ++changed;
    }
    catalog_.save();
    return ok_json("dml", std::to_string(changed) + " row(s) updated", changed);
}

std::string SqlEngine::delete_rows(const std::string& sql) {
    std::regex re("DELETE\\s+FROM\\s+" + ident_regex() + "(.*)$", std::regex_constants::icase);
    std::smatch m;
    if (!std::regex_match(sql, m, re)) throw std::runtime_error("Invalid DELETE syntax");
    std::string table_name = m[1].str();
    std::string tail = trim(m[2].str());
    std::string where;
    auto where_pos = find_keyword_top(tail, "WHERE");
    if (where_pos != std::string::npos) where = trim(tail.substr(where_pos + 5));
    auto& table = catalog_.table(table_name);
    int removed = 0;
    std::vector<std::vector<std::string>> kept;
    for (const auto& row : table.rows) {
        if (row_matches(table, row, where)) ++removed;
        else kept.push_back(row);
    }
    table.rows = std::move(kept);
    catalog_.save();
    return ok_json("dml", std::to_string(removed) + " row(s) deleted", removed);
}

std::string SqlEngine::show_items(const std::string& sql) {
    auto u = upper(sql);
    if (u.find("DATABASES") != std::string::npos) {
        std::vector<std::vector<std::string>> rows;
        for (auto& name : catalog_.database_names()) rows.push_back({name});
        return select_json({"database"}, rows);
    }
    if (u.find("TABLES") != std::string::npos) {
        std::vector<std::vector<std::string>> rows;
        for (auto& name : catalog_.table_names(catalog_.current_database())) rows.push_back({name});
        return select_json({"table"}, rows);
    }
    throw std::runtime_error("Unsupported SHOW command");
}

} // namespace db
