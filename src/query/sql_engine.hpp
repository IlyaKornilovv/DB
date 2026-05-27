#pragma once
#include "../storage/catalog_store.hpp"
#include <filesystem>
#include <string>

namespace db {

class SqlEngine {
public:
    explicit SqlEngine(std::filesystem::path data_root);
    std::string execute_batch(const std::string& sql);

private:
    CatalogStore catalog_;

    std::string execute_one(const std::string& sql);
    std::string create_database(const std::string& sql);
    std::string drop_database(const std::string& sql);
    std::string use_database(const std::string& sql);
    std::string create_table(const std::string& sql);
    std::string drop_table(const std::string& sql);
    std::string insert_into(const std::string& sql);
    std::string select_from(const std::string& sql);
    std::string update_rows(const std::string& sql);
    std::string delete_rows(const std::string& sql);
    std::string show_items(const std::string& sql);
};

} // namespace db
