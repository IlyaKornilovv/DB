#pragma once

#include <filesystem>
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace db {

enum class FieldKind { Int, Float, Bool, Text, Varchar, TextArray };

struct ColumnSpec {
    std::string name;
    FieldKind kind{FieldKind::Text};
    int max_len{0};
    bool auto_increment{false};
    bool unique{false};
    long long next_value{1};
};

struct TableData {
    std::string name;
    std::vector<ColumnSpec> columns;
    std::vector<std::vector<std::string>> rows;
};

struct DatabaseData {
    std::string name;
    std::map<std::string, TableData> tables;
};

class CatalogStore {
public:
    explicit CatalogStore(std::filesystem::path root);

    void load();
    void save() const;

    bool has_database(const std::string& name) const;
    bool has_table(const std::string& db_name, const std::string& table_name) const;
    void create_database(const std::string& name);
    void drop_database(const std::string& name);
    void use_database(const std::string& name);
    const std::string& current_database() const;

    std::vector<std::string> database_names() const;
    std::vector<std::string> table_names(const std::string& db_name) const;

    void create_table(const std::string& table_name, std::vector<ColumnSpec> columns);
    void drop_table(const std::string& table_name);

    TableData& table(const std::string& table_name);
    const TableData& table(const std::string& table_name) const;

    std::mutex& mutex() { return mutex_; }

private:
    std::filesystem::path root_;
    std::map<std::string, DatabaseData> databases_;
    std::string active_db_;
    mutable std::mutex mutex_;

    DatabaseData& active_database();
    const DatabaseData& active_database() const;
};

std::string kind_to_text(FieldKind kind);
FieldKind kind_from_text(const std::string& text);
std::string escape_cell(const std::string& value);
std::string unescape_cell(const std::string& value);

} // namespace db
