#include "catalog_store.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace db {
namespace {
std::vector<std::string> split_pipe(const std::string& line) {
    std::vector<std::string> out;
    std::string cur;
    bool escaped = false;
    for (char ch : line) {
        if (escaped) {
            cur.push_back('\\');
            cur.push_back(ch);
            escaped = false;
        } else if (ch == '\\') {
            escaped = true;
        } else if (ch == '|') {
            out.push_back(cur);
            cur.clear();
        } else {
            cur.push_back(ch);
        }
    }
    if (escaped) cur.push_back('\\');
    out.push_back(cur);
    return out;
}

std::string db_file_name(const std::string& name) {
    return name + ".store";
}

} // namespace

std::string kind_to_text(FieldKind kind) {
    switch (kind) {
        case FieldKind::Int: return "INT";
        case FieldKind::Float: return "FLOAT";
        case FieldKind::Bool: return "BOOL";
        case FieldKind::Text: return "TEXT";
        case FieldKind::Varchar: return "VARCHAR";
        case FieldKind::TextArray: return "TEXT[]";
    }
    return "TEXT";
}

FieldKind kind_from_text(const std::string& text) {
    if (text == "INT" || text == "INTEGER") return FieldKind::Int;
    if (text == "FLOAT" || text == "DOUBLE") return FieldKind::Float;
    if (text == "BOOL" || text == "BOOLEAN") return FieldKind::Bool;
    if (text == "VARCHAR") return FieldKind::Varchar;
    if (text == "TEXT[]") return FieldKind::TextArray;
    return FieldKind::Text;
}

std::string escape_cell(const std::string& value) {
    std::string out;
    for (char ch : value) {
        switch (ch) {
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '|': out += "\\p"; break;
            default: out += ch; break;
        }
    }
    return out;
}

std::string unescape_cell(const std::string& value) {
    std::string out;
    bool escaped = false;
    for (char ch : value) {
        if (!escaped) {
            if (ch == '\\') escaped = true;
            else out.push_back(ch);
            continue;
        }
        if (ch == 'n') out.push_back('\n');
        else if (ch == 'r') out.push_back('\r');
        else if (ch == 'p') out.push_back('|');
        else out.push_back(ch);
        escaped = false;
    }
    if (escaped) out.push_back('\\');
    return out;
}

CatalogStore::CatalogStore(std::filesystem::path root) : root_(std::move(root)) {}

void CatalogStore::load() {
    std::filesystem::create_directories(root_);
    databases_.clear();
    active_db_.clear();

    for (const auto& entry : std::filesystem::directory_iterator(root_)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".store") continue;
        std::ifstream in(entry.path());
        DatabaseData db_data;
        TableData* current_table = nullptr;
        std::string line;
        while (std::getline(in, line)) {
            if (line.empty()) continue;
            auto parts = split_pipe(line);
            if (parts.empty()) continue;
            const std::string tag = parts[0];
            if (tag == "DB" && parts.size() >= 2) {
                db_data.name = unescape_cell(parts[1]);
            } else if (tag == "ACTIVE" && parts.size() >= 2 && parts[1] == "1") {
                active_db_ = db_data.name;
            } else if (tag == "TABLE" && parts.size() >= 2) {
                TableData table;
                table.name = unescape_cell(parts[1]);
                db_data.tables[table.name] = table;
                current_table = &db_data.tables[table.name];
            } else if (tag == "COL" && current_table && parts.size() >= 7) {
                ColumnSpec c;
                c.name = unescape_cell(parts[1]);
                c.kind = kind_from_text(parts[2]);
                c.max_len = std::stoi(parts[3]);
                c.auto_increment = parts[4] == "1";
                c.unique = parts[5] == "1";
                c.next_value = std::stoll(parts[6]);
                current_table->columns.push_back(c);
            } else if (tag == "ROW" && current_table) {
                std::vector<std::string> row;
                for (size_t i = 1; i < parts.size(); ++i) row.push_back(unescape_cell(parts[i]));
                current_table->rows.push_back(row);
            } else if (tag == "ENDTABLE") {
                current_table = nullptr;
            }
        }
        if (!db_data.name.empty()) databases_[db_data.name] = std::move(db_data);
    }
    if (active_db_.empty() && !databases_.empty()) active_db_ = databases_.begin()->first;
}

void CatalogStore::save() const {
    std::filesystem::create_directories(root_);
    for (const auto& entry : std::filesystem::directory_iterator(root_)) {
        if (entry.is_regular_file() && entry.path().extension() == ".store") std::filesystem::remove(entry.path());
    }
    for (const auto& [db_name, db_data] : databases_) {
        std::ofstream out(root_ / db_file_name(db_name), std::ios::trunc);
        out << "DB|" << escape_cell(db_data.name) << "\n";
        out << "ACTIVE|" << (db_name == active_db_ ? "1" : "0") << "\n";
        for (const auto& [table_name, table] : db_data.tables) {
            out << "TABLE|" << escape_cell(table.name) << "\n";
            for (const auto& c : table.columns) {
                out << "COL|" << escape_cell(c.name) << "|" << kind_to_text(c.kind) << "|" << c.max_len
                    << "|" << (c.auto_increment ? "1" : "0") << "|" << (c.unique ? "1" : "0")
                    << "|" << c.next_value << "\n";
            }
            for (const auto& row : table.rows) {
                out << "ROW";
                for (const auto& cell : row) out << "|" << escape_cell(cell);
                out << "\n";
            }
            out << "ENDTABLE\n";
        }
    }
}

bool CatalogStore::has_database(const std::string& name) const { return databases_.count(name) != 0; }

bool CatalogStore::has_table(const std::string& db_name, const std::string& table_name) const {
    auto db_it = databases_.find(db_name);
    return db_it != databases_.end() && db_it->second.tables.count(table_name) != 0;
}

void CatalogStore::create_database(const std::string& name) {
    if (has_database(name)) throw std::runtime_error("Database already exists: " + name);
    DatabaseData db;
    db.name = name;
    databases_[name] = std::move(db);
    active_db_ = name;
}

void CatalogStore::drop_database(const std::string& name) {
    if (!has_database(name)) throw std::runtime_error("Database not found: " + name);
    databases_.erase(name);
    if (active_db_ == name) active_db_ = databases_.empty() ? std::string{} : databases_.begin()->first;
}

void CatalogStore::use_database(const std::string& name) {
    if (!has_database(name)) throw std::runtime_error("Database not found: " + name);
    active_db_ = name;
}

const std::string& CatalogStore::current_database() const { return active_db_; }

std::vector<std::string> CatalogStore::database_names() const {
    std::vector<std::string> names;
    for (const auto& [name, _] : databases_) names.push_back(name);
    return names;
}

std::vector<std::string> CatalogStore::table_names(const std::string& db_name) const {
    std::vector<std::string> names;
    const auto& db = databases_.at(db_name);
    for (const auto& [name, _] : db.tables) names.push_back(name);
    return names;
}

DatabaseData& CatalogStore::active_database() {
    if (active_db_.empty()) throw std::runtime_error("No database selected");
    auto it = databases_.find(active_db_);
    if (it == databases_.end()) throw std::runtime_error("Active database is missing");
    return it->second;
}

const DatabaseData& CatalogStore::active_database() const {
    if (active_db_.empty()) throw std::runtime_error("No database selected");
    auto it = databases_.find(active_db_);
    if (it == databases_.end()) throw std::runtime_error("Active database is missing");
    return it->second;
}

void CatalogStore::create_table(const std::string& table_name, std::vector<ColumnSpec> columns) {
    auto& db = active_database();
    if (db.tables.count(table_name)) throw std::runtime_error("Table already exists: " + table_name);
    TableData table;
    table.name = table_name;
    table.columns = std::move(columns);
    db.tables[table_name] = std::move(table);
}

void CatalogStore::drop_table(const std::string& table_name) {
    auto& db = active_database();
    if (!db.tables.erase(table_name)) throw std::runtime_error("Table not found: " + table_name);
}

TableData& CatalogStore::table(const std::string& table_name) {
    auto& db = active_database();
    auto it = db.tables.find(table_name);
    if (it == db.tables.end()) throw std::runtime_error("Table not found: " + table_name);
    return it->second;
}

const TableData& CatalogStore::table(const std::string& table_name) const {
    const auto& db = active_database();
    auto it = db.tables.find(table_name);
    if (it == db.tables.end()) throw std::runtime_error("Table not found: " + table_name);
    return it->second;
}

} // namespace db
