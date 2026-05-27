#include "socket_compat.hpp"
#include <cstring>

namespace db::net {

SocketRuntime::SocketRuntime() {
#ifdef _WIN32
    WSADATA data;
    WSAStartup(MAKEWORD(2, 2), &data);
#endif
}

SocketRuntime::~SocketRuntime() {
#ifdef _WIN32
    WSACleanup();
#endif
}

void close_socket(socket_handle s) {
#ifdef _WIN32
    closesocket(s);
#else
    close(s);
#endif
}

bool send_all(socket_handle s, const void* data, size_t bytes) {
    const char* ptr = static_cast<const char*>(data);
    while (bytes > 0) {
#ifdef _WIN32
        int sent = send(s, ptr, static_cast<int>(bytes), 0);
#else
        ssize_t sent = send(s, ptr, bytes, 0);
#endif
        if (sent <= 0) return false;
        ptr += sent;
        bytes -= static_cast<size_t>(sent);
    }
    return true;
}

bool recv_all(socket_handle s, void* data, size_t bytes) {
    char* ptr = static_cast<char*>(data);
    while (bytes > 0) {
#ifdef _WIN32
        int got = recv(s, ptr, static_cast<int>(bytes), 0);
#else
        ssize_t got = recv(s, ptr, bytes, 0);
#endif
        if (got <= 0) return false;
        ptr += got;
        bytes -= static_cast<size_t>(got);
    }
    return true;
}

uint32_t read_be32(const unsigned char* data) {
    return (static_cast<uint32_t>(data[0]) << 24) |
           (static_cast<uint32_t>(data[1]) << 16) |
           (static_cast<uint32_t>(data[2]) << 8) |
           static_cast<uint32_t>(data[3]);
}

void write_be32(uint32_t value, unsigned char* data) {
    data[0] = static_cast<unsigned char>((value >> 24) & 0xff);
    data[1] = static_cast<unsigned char>((value >> 16) & 0xff);
    data[2] = static_cast<unsigned char>((value >> 8) & 0xff);
    data[3] = static_cast<unsigned char>(value & 0xff);
}

} // namespace db::net
