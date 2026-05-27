#include "../src/query/sql_engine.hpp"
#include "../src/query/text_tools.hpp"
#include <cassert>
#include <filesystem>
#include <iostream>

int main() {
    using namespace db;
    auto parts = split_statements("CREATE DATABASE a; INSERT INTO t VALUES ('x;y'); SELECT * FROM t;");
    assert(parts.size() == 3);
    assert(strip_quotes("'Alice'") == "Alice");

    auto temp = std::filesystem::temp_directory_path() / "db_unit_test_case";
    std::filesystem::remove_all(temp);
    SqlEngine engine(temp);
    auto r1 = engine.execute_batch("CREATE DATABASE test; CREATE TABLE users (id INT AUTO_INCREMENT, name TEXT UNIQUE); INSERT INTO users (name) VALUES ('Ann'); SELECT * FROM users;");
    assert(r1.find("\"type\":\"select\"") != std::string::npos);
    assert(r1.find("Ann") != std::string::npos);
    auto dup = engine.execute_batch("INSERT INTO users (name) VALUES ('Ann');");
    assert(dup.find("\"type\":\"error\"") != std::string::npos);
    std::filesystem::remove_all(temp);
    std::cout << "unit_tests passed\n";
    return 0;
}
