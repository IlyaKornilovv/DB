#include "../../include/db/native_client.h"
#include "../wire/socket_compat.hpp"

#include <cstring>
#include <memory>
#include <string>

namespace {
struct DbConnection {
    std::string host;
    int port;
};

std::string execute_tcp(const std::string& host, int port, const std::string& query) {
    static db::net::SocketRuntime runtime;
    socket_handle fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == invalid_socket_value) return R"({"type":"error","message":"socket failed"})";
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(port));
    std::string ip = host.empty() ? "127.0.0.1" : host;
    if (ip == "localhost") ip = "127.0.0.1";
    if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) != 1) {
        db::net::close_socket(fd);
        return R"({"type":"error","message":"invalid host"})";
    }
    if (connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        db::net::close_socket(fd);
        return R"({"type":"error","message":"connect failed"})";
    }
    unsigned char len[4]{};
    db::net::write_be32(static_cast<uint32_t>(query.size()), len);
    if (!db::net::send_all(fd, len, 4) || !db::net::send_all(fd, query.data(), query.size())) {
        db::net::close_socket(fd);
        return R"({"type":"error","message":"send failed"})";
    }
    unsigned char status[4]{};
    unsigned char data_len[4]{};
    if (!db::net::recv_all(fd, status, 4) || !db::net::recv_all(fd, data_len, 4)) {
        db::net::close_socket(fd);
        return R"({"type":"error","message":"receive failed"})";
    }
    uint32_t n = db::net::read_be32(data_len);
    std::string response(n, '\0');
    if (n && !db::net::recv_all(fd, response.data(), n)) {
        db::net::close_socket(fd);
        return R"({"type":"error","message":"receive body failed"})";
    }
    db::net::close_socket(fd);
    return response;
}

char* copy_to_c_string(const std::string& text) {
    char* out = static_cast<char*>(std::malloc(text.size() + 1));
    if (!out) return nullptr;
    std::memcpy(out, text.c_str(), text.size() + 1);
    return out;
}
}

extern "C" {

void* db_connect(const char* host, int port) {
    auto* conn = new DbConnection;
    conn->host = host ? host : "127.0.0.1";
    conn->port = port > 0 ? port : 5432;
    return conn;
}

const char* db_execute(void* connection, const char* query) {
    if (!connection) return copy_to_c_string(R"({"type":"error","message":"null connection"})");
    auto* conn = static_cast<DbConnection*>(connection);
    return copy_to_c_string(execute_tcp(conn->host, conn->port, query ? query : ""));
}

void db_disconnect(void* connection) {
    delete static_cast<DbConnection*>(connection);
}

void db_free_string(const char* str) {
    std::free(const_cast<char*>(str));
}

}
