#pragma once

#include "plc/core/types.hpp"
#include "plc/core/result.hpp"
#include "plc/network/socket.hpp"
#include "plc/modbus/modbus_defs.hpp"

#include <vector>
#include <memory>
#include <atomic>

namespace plc::modbus {

class IModbusClient {
public:
    virtual ~IModbusClient() = default;
    virtual core::Result<void> connect() = 0;
    virtual void disconnect() noexcept = 0;
    [[nodiscard]] virtual bool is_connected() const noexcept = 0;
    virtual core::Result<std::vector<std::int16_t>> read_holding_registers(
        core::RegisterAddress start,
        core::RegisterCount count) = 0;
    [[nodiscard]] virtual const core::ConnectionConfig& config() const noexcept = 0;
};

class ModbusTcpClient : public IModbusClient {
public:
    explicit ModbusTcpClient(core::ConnectionConfig config,
                             std::unique_ptr<network::ISocket> socket = nullptr);
    ~ModbusTcpClient() override;

    ModbusTcpClient(const ModbusTcpClient&) = delete;
    ModbusTcpClient& operator=(const ModbusTcpClient&) = delete;

    ModbusTcpClient(ModbusTcpClient&&) noexcept;
    ModbusTcpClient& operator=(ModbusTcpClient&&) noexcept;

    core::Result<void> connect() override;
    void disconnect() noexcept override;
    [[nodiscard]] bool is_connected() const noexcept override;

    core::Result<std::vector<std::int16_t>> read_holding_registers(
        core::RegisterAddress start,
        core::RegisterCount count) override;

    [[nodiscard]] const core::ConnectionConfig& config() const noexcept override {
        return config_;
    }

private:
    core::ConnectionConfig config_;
    std::unique_ptr<network::ISocket> socket_;
    std::atomic<core::TransactionId> next_tx_id_{1};
};

} // namespace plc::modbus
