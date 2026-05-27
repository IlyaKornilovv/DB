#include "server_runtime.hpp"
#include <iostream>
#include <stdexcept>

namespace db::net {

TcpSqlServer::TcpSqlServer(std::string host, int port, std::filesystem::path data_root)
    : host_(std::move(host)), port_(port), engine_(std::move(data_root)) {}

void TcpSqlServer::run_forever() {
    socket_handle server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == invalid_socket_value) throw std::runtime_error("socket() failed");

    int yes = 1;
#ifdef _WIN32
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&yes), sizeof(yes));
#else
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
#endif

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(port_));
    if (host_.empty() || host_ == "0.0.0.0" || host_ == "*") addr.sin_addr.s_addr = htonl(INADDR_ANY);
    else if (inet_pton(AF_INET, host_.c_str(), &addr.sin_addr) != 1) {
        close_socket(server_fd);
        throw std::runtime_error("invalid listen address: " + host_);
    }

    if (bind(server_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        close_socket(server_fd);
        throw std::runtime_error("bind() failed; port may be busy");
    }
    if (listen(server_fd, 32) < 0) {
        close_socket(server_fd);
        throw std::runtime_error("listen() failed");
    }
    std::cout << "DB server listening on " << (host_.empty() ? "0.0.0.0" : host_) << ':' << port_ << std::endl;

    while (true) {
        sockaddr_in peer{};
#ifdef _WIN32
        int peer_len = sizeof(peer);
#else
        socklen_t peer_len = sizeof(peer);
#endif
        socket_handle client = accept(server_fd, reinterpret_cast<sockaddr*>(&peer), &peer_len);
        if (client == invalid_socket_value) continue;
        std::thread(&TcpSqlServer::serve_client, this, client).detach();
    }
}

void TcpSqlServer::serve_client(socket_handle client) {
    auto guard = std::unique_ptr<void, void(*)(void*)>(reinterpret_cast<void*>(static_cast<intptr_t>(client)), [](void* p) {
        close_socket(static_cast<socket_handle>(reinterpret_cast<intptr_t>(p)));
    });
    unsigned char len_buf[4]{};
    if (!recv_all(client, len_buf, 4)) return;
    uint32_t len = read_be32(len_buf);
    if (len > 10 * 1024 * 1024) return;
    std::string query(len, '\0');
    if (len && !recv_all(client, query.data(), len)) return;

    std::string response = engine_.execute_batch(query);
    unsigned char status[4]{};
    write_be32(response.find("\"type\":\"error\"") == std::string::npos ? 0 : 1, status);
    unsigned char out_len[4]{};
    write_be32(static_cast<uint32_t>(response.size()), out_len);
    send_all(client, status, 4);
    send_all(client, out_len, 4);
    if (!response.empty()) send_all(client, response.data(), response.size());
}

} // namespace db::net
