#include "../src/query/sql_engine.hpp"
#include <cassert>
#include <filesystem>
#include <iostream>

int main() {
    auto temp = std::filesystem::temp_directory_path() / "db_integration_test_case";
    std::filesystem::remove_all(temp);
    {
        db::SqlEngine engine(temp);
        auto out = engine.execute_batch("CREATE DATABASE persist; USE persist; CREATE TABLE items (id INT, title TEXT); INSERT INTO items VALUES (1, 'saved');");
        assert(out.find("dml") != std::string::npos);
    }
    {
        db::SqlEngine engine(temp);
        auto out = engine.execute_batch("USE persist; SELECT * FROM items WHERE id = 1;");
        assert(out.find("\"type\":\"select\"") != std::string::npos);
        assert(out.find("saved") != std::string::npos);
    }
    std::filesystem::remove_all(temp);
    std::cout << "integration_tests passed\n";
    return 0;
}
