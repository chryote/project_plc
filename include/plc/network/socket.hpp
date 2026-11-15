#pragma once

#include "plc/core/types.hpp"
#include "plc/core/result.hpp"

#include <string>
#include <vector>
#include <chrono>
#include <memory>

namespace plc::network {

class ISocket {
public:
    virtual ~ISocket() = default;
    virtual core::Result<void> connect(const std::string& host, std::uint16_t port, core::Milliseconds timeout) = 0;
    virtual core::Result<void> send_all(const core::Byte* data, std::size_t size) = 0;
    virtual core::Result<void> receive_exact(core::Byte* buffer, std::size_t size) = 0;
    virtual void close() noexcept = 0;
    [[nodiscard]] virtual bool is_connected() const noexcept = 0;
};

class TcpSocket : public ISocket {
public:
    TcpSocket();
    ~TcpSocket() override;

    TcpSocket(const TcpSocket&) = delete;
    TcpSocket& operator=(const TcpSocket&) = delete;

    TcpSocket(TcpSocket&& other) noexcept;
    TcpSocket& operator=(TcpSocket&& other) noexcept;

    core::Result<void> connect(const std::string& host, std::uint16_t port, core::Milliseconds timeout) override;
    core::Result<void> send_all(const core::Byte* data, std::size_t size) override;
    core::Result<void> receive_exact(core::Byte* buffer, std::size_t size) override;
    void close() noexcept override;
    [[nodiscard]] bool is_connected() const noexcept override;

    void set_timeout(core::Milliseconds timeout);

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace plc::network
