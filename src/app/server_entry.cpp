#include "../wire/server_runtime.hpp"
#include <filesystem>
#include <iostream>
#include <string>

namespace {
std::filesystem::path default_data_path() {
    auto cwd = std::filesystem::current_path();
    if (cwd.filename() == "build" && std::filesystem::exists(cwd.parent_path() / "CMakeLists.txt")) {
        return cwd.parent_path() / "data";
    }
    return cwd / "data";
}
}

int main(int argc, char** argv) {
    std::string host = "0.0.0.0";
    int port = 5432;
    auto data = default_data_path();

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if ((arg == "--host" || arg == "-h") && i + 1 < argc) host = argv[++i];
        else if ((arg == "--port" || arg == "-p") && i + 1 < argc) port = std::stoi(argv[++i]);
        else if ((arg == "--data" || arg == "-d") && i + 1 < argc) data = argv[++i];
        else if (arg == "--help") {
            std::cout << "DB server\nUsage: customdb_server [--host 0.0.0.0] [--port 5432] [--data ./data]\n";
            return 0;
        }
    }

    try {
        db::net::TcpSqlServer server(host, port, data);
        server.run_forever();
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
