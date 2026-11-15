#include "plc/network/socket.hpp"
#include "plc/core/logger.hpp"

#if defined(_WIN32)
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32.lib")
  using socket_t = SOCKET;
  constexpr socket_t INVALID_SOCK = INVALID_SOCKET;
  #define CLOSE_SOCKET(s) closesocket(s)
  #define LAST_ERROR() WSAGetLastError()
  #define IS_WOULDBLOCK(err) ((err) == WSAEWOULDBLOCK || (err) == WSAEINPROGRESS)
#else
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <netinet/tcp.h>
  #include <arpa/inet.h>
  #include <netdb.h>
  #include <unistd.h>
  #include <fcntl.h>
  #include <errno.h>
  using socket_t = int;
  constexpr socket_t INVALID_SOCK = -1;
  #define CLOSE_SOCKET(s) ::close(s)
  #define LAST_ERROR() errno
  #define IS_WOULDBLOCK(err) ((err) == EAGAIN || (err) == EWOULDBLOCK || (err) == EINPROGRESS)
#endif

namespace plc::network {

namespace {

#if defined(_WIN32)
struct WinsockInit {
    WinsockInit() {
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
            LOG_ERROR("WSAStartup failed");
        }
    }
    ~WinsockInit() {
        WSACleanup();
    }
};

void ensure_winsock() {
    static WinsockInit init;
}
#else
void ensure_winsock() {}
#endif

bool set_socket_blocking(socket_t sock, bool blocking) {
#if defined(_WIN32)
    u_long mode = blocking ? 0 : 1;
    return ioctlsocket(sock, FIONBIO, &mode) == 0;
#else
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags < 0) return false;
    flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
    return fcntl(sock, F_SETFL, flags) == 0;
#endif
}

} // namespace

struct TcpSocket::Impl {
    socket_t sock = INVALID_SOCK;
    core::Milliseconds timeout{3000};
    bool connected = false;

    ~Impl() {
        cleanup();
    }

    void cleanup() noexcept {
        if (sock != INVALID_SOCK) {
            CLOSE_SOCKET(sock);
            sock = INVALID_SOCK;
        }
        connected = false;
    }
};

TcpSocket::TcpSocket() : pimpl_(std::make_unique<Impl>()) {
    ensure_winsock();
}

TcpSocket::~TcpSocket() = default;

TcpSocket::TcpSocket(TcpSocket&& other) noexcept = default;
TcpSocket& TcpSocket::operator=(TcpSocket&& other) noexcept = default;

bool TcpSocket::is_connected() const noexcept {
    return pimpl_ && pimpl_->connected && pimpl_->sock != INVALID_SOCK;
}

void TcpSocket::close() noexcept {
    if (pimpl_) {
        pimpl_->cleanup();
    }
}

void TcpSocket::set_timeout(core::Milliseconds timeout) {
    if (!pimpl_) return;
    pimpl_->timeout = timeout;

    if (pimpl_->sock != INVALID_SOCK) {
#if defined(_WIN32)
        DWORD tv = static_cast<DWORD>(timeout.count());
        setsockopt(pimpl_->sock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&tv), sizeof(tv));
        setsockopt(pimpl_->sock, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&tv), sizeof(tv));
#else
        struct timeval tv;
        tv.tv_sec = static_cast<time_t>(timeout.count() / 1000);
        tv.tv_usec = static_cast<suseconds_t>((timeout.count() % 1000) * 1000);
        setsockopt(pimpl_->sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        setsockopt(pimpl_->sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
#endif
    }
}

core::Result<void> TcpSocket::connect(const std::string& host, std::uint16_t port, core::Milliseconds timeout) {
    close();

    pimpl_->timeout = timeout;

    struct addrinfo hints{};
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    struct addrinfo* result = nullptr;
    const std::string port_str = std::to_string(port);

    int res = getaddrinfo(host.c_str(), port_str.c_str(), &hints, &result);
    if (res != 0 || !result) {
        return core::Error{core::StatusCode::ConnectionFailed, "Failed to resolve host " + host, res};
    }

    socket_t s = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (s == INVALID_SOCK) {
        int err = LAST_ERROR();
        freeaddrinfo(result);
        return core::Error{core::StatusCode::SocketError, "Socket creation failed", err};
    }

    // Set non-blocking for connect timeout
    set_socket_blocking(s, false);

    int conn_res = ::connect(s, result->ai_addr, static_cast<int>(result->ai_addrlen));
    if (conn_res < 0) {
        int err = LAST_ERROR();
        if (!IS_WOULDBLOCK(err)) {
            CLOSE_SOCKET(s);
            freeaddrinfo(result);
            return core::Error{core::StatusCode::ConnectionFailed, "Connect error immediately", err};
        }

        // Wait for connect completion via select
        fd_set write_fds;
        fd_set err_fds;
        FD_ZERO(&write_fds);
        FD_ZERO(&err_fds);
        FD_SET(s, &write_fds);
        FD_SET(s, &err_fds);

        struct timeval tv;
        tv.tv_sec = static_cast<long>(timeout.count() / 1000);
        tv.tv_usec = static_cast<long>((timeout.count() % 1000) * 1000);

        int select_res = select(static_cast<int>(s + 1), nullptr, &write_fds, &err_fds, &tv);
        if (select_res <= 0) {
            CLOSE_SOCKET(s);
            freeaddrinfo(result);
            if (select_res == 0) {
                return core::Error{core::StatusCode::ConnectionTimeout, "Connection timed out to " + host + ":" + port_str, 0};
            }
            return core::Error{core::StatusCode::ConnectionFailed, "Connection select failed", LAST_ERROR()};
        }

        int sock_err = 0;
        socklen_t optlen = sizeof(sock_err);
        if (getsockopt(s, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&sock_err), &optlen) < 0 || sock_err != 0) {
            CLOSE_SOCKET(s);
            freeaddrinfo(result);
            return core::Error{core::StatusCode::ConnectionFailed, "Connect socket error: " + std::to_string(sock_err), sock_err};
        }
    }

    freeaddrinfo(result);

    // Restore blocking mode
    set_socket_blocking(s, true);

    // Set TCP_NODELAY to avoid Nagle algorithm latency
    int flag = 1;
    setsockopt(s, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&flag), sizeof(flag));

    pimpl_->sock = s;
    pimpl_->connected = true;

    set_timeout(timeout);

    return {};
}

core::Result<void> TcpSocket::send_all(const core::Byte* data, std::size_t size) {
    if (!is_connected()) {
        return core::Error{core::StatusCode::SocketError, "Socket not connected", 0};
    }

    std::size_t total_sent = 0;
    while (total_sent < size) {
        int sent = ::send(pimpl_->sock, reinterpret_cast<const char*>(data + total_sent),
                          static_cast<int>(size - total_sent), 0);
        if (sent <= 0) {
            int err = LAST_ERROR();
            pimpl_->cleanup();
            return core::Error{core::StatusCode::SendFailed, "Failed to send data over TCP socket", err};
        }
        total_sent += static_cast<std::size_t>(sent);
    }

    return {};
}

core::Result<void> TcpSocket::receive_exact(core::Byte* buffer, std::size_t size) {
    if (!is_connected()) {
        return core::Error{core::StatusCode::SocketError, "Socket not connected", 0};
    }

    std::size_t total_received = 0;
    while (total_received < size) {
        int received = ::recv(pimpl_->sock, reinterpret_cast<char*>(buffer + total_received),
                              static_cast<int>(size - total_received), 0);
        if (received <= 0) {
            int err = LAST_ERROR();
            pimpl_->cleanup();
            if (received == 0) {
                return core::Error{core::StatusCode::ReceiveFailed, "Connection closed by remote peer", 0};
            }
            return core::Error{core::StatusCode::ReceiveFailed, "Receive error on socket", err};
        }
        total_received += static_cast<std::size_t>(received);
    }

    return {};
}

} // namespace plc::network
