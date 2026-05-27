#pragma once
#include <cstdint>
#include <string>

#ifdef _WIN32
  #ifndef NOMINMAX
  #define NOMINMAX
  #endif
  #include <winsock2.h>
  #include <ws2tcpip.h>
  using socket_handle = SOCKET;
  constexpr socket_handle invalid_socket_value = INVALID_SOCKET;
#else
  #include <arpa/inet.h>
  #include <netinet/in.h>
  #include <sys/socket.h>
  #include <unistd.h>
  using socket_handle = int;
  constexpr socket_handle invalid_socket_value = -1;
#endif

namespace db::net {

class SocketRuntime {
public:
    SocketRuntime();
    ~SocketRuntime();
    SocketRuntime(const SocketRuntime&) = delete;
    SocketRuntime& operator=(const SocketRuntime&) = delete;
};

void close_socket(socket_handle s);
bool send_all(socket_handle s, const void* data, size_t bytes);
bool recv_all(socket_handle s, void* data, size_t bytes);
uint32_t read_be32(const unsigned char* data);
void write_be32(uint32_t value, unsigned char* data);

} // namespace db::net
