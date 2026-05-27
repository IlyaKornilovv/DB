#pragma once
#include "../query/sql_engine.hpp"
#include "socket_compat.hpp"
#include <atomic>
#include <filesystem>
#include <memory>
#include <thread>
#include <vector>

namespace db::net {

class TcpSqlServer {
public:
    TcpSqlServer(std::string host, int port, std::filesystem::path data_root);
    void run_forever();

private:
    std::string host_;
    int port_;
    SqlEngine engine_;
    SocketRuntime runtime_;

    void serve_client(socket_handle client);
};

} // namespace db::net
