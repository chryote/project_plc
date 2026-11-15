#pragma once

#include "plc/modbus/modbus_client.hpp"
#include <map>
#include <mutex>
#include <thread>
#include <optional>

namespace plc::modbus {

class MockModbusClient : public IModbusClient {
public:
    explicit MockModbusClient(core::ConnectionConfig config = {})
        : config_(std::move(config)) {
        initialize_default_registers();
    }

    core::Result<void> connect() override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (fail_connection_) {
            return core::Error{core::StatusCode::ConnectionFailed, "Mock simulated connection failure"};
        }
        connected_ = true;
        return {};
    }

    void disconnect() noexcept override {
        std::lock_guard<std::mutex> lock(mutex_);
        connected_ = false;
    }

    [[nodiscard]] bool is_connected() const noexcept override {
        std::lock_guard<std::mutex> lock(mutex_);
        return connected_;
    }

    core::Result<std::vector<std::int16_t>> read_holding_registers(
        core::RegisterAddress start,
        core::RegisterCount count) override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!connected_) {
            return core::Error{core::StatusCode::SocketError, "Mock client not connected"};
        }

        if (simulated_latency_.count() > 0) {
            std::this_thread::sleep_for(simulated_latency_);
        }

        if (injected_exception_.has_value()) {
            return core::Error{core::StatusCode::ModbusException,
                "Mock Modbus Exception: " + exception_to_string(*injected_exception_)};
        }

        std::vector<std::int16_t> values;
        values.reserve(count);

        for (core::RegisterCount i = 0; i < count; ++i) {
            core::RegisterAddress addr = start + i;
            auto it = registers_.find(addr);
            if (it != registers_.end()) {
                values.push_back(it->second);
            } else {
                values.push_back(0); // Default to 0 for uninitialized registers
            }
        }

        return values;
    }

    [[nodiscard]] const core::ConnectionConfig& config() const noexcept override {
        return config_;
    }

    // Mock configuration helpers
    void set_register(core::RegisterAddress addr, std::int16_t val) {
        std::lock_guard<std::mutex> lock(mutex_);
        registers_[addr] = val;
    }

    void set_registers(core::RegisterAddress start_addr, const std::vector<std::int16_t>& vals) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (std::size_t i = 0; i < vals.size(); ++i) {
            registers_[static_cast<core::RegisterAddress>(start_addr + i)] = vals[i];
        }
    }

    [[nodiscard]] std::int16_t get_register(core::RegisterAddress addr) {
        std::lock_guard<std::mutex> lock(mutex_);
        return registers_[addr];
    }

    void inject_exception(std::optional<ExceptionCode> exc) {
        std::lock_guard<std::mutex> lock(mutex_);
        injected_exception_ = exc;
    }

    void set_fail_connection(bool fail) {
        std::lock_guard<std::mutex> lock(mutex_);
        fail_connection_ = fail;
    }

    void set_latency(core::Milliseconds ms) {
        std::lock_guard<std::mutex> lock(mutex_);
        simulated_latency_ = ms;
    }

    void initialize_default_registers() {
        // Direct 0-based addresses (from main.py)
        registers_[1110] = 131;    // V1110 (Side Slip raw)
        registers_[1100] = 0;      // V1100 (Side Slip scaled)
        registers_[1102] = 130;    // V1102 (Raw Low)
        registers_[1103] = 30;     // V1103 (Raw High)
        registers_[1104] = -1;     // V1104 (Disp Low)
        registers_[1105] = 30;     // V1105 (Disp High)

        registers_[1230] = 36;     // V1230 (CH1 Avg)
        registers_[1231] = 0;      // V1231
        registers_[1232] = 37;     // V1232 (CH1 Live)

        registers_[1260] = 1597;   // V1260 (CH2 Avg)
        registers_[1261] = 0;      // V1261
        registers_[1262] = 1594;   // V1262 (CH2 Live)

        registers_[50] = 1200;     // Sumbu 1
        registers_[51] = 1250;     // Sumbu 2
        registers_[52] = 0;        // Sumbu 3
        registers_[53] = 0;        // Sumbu 4

        registers_[1300] = 2820;   // Display speed
        registers_[1310] = 360;    // PPR
        registers_[1320] = 0;      // Raw pulses
        registers_[1322] = 0;      // Pulse per min

        // Offset 512 + n addresses (Haiwell base offset mapping)
        // AI0 -> 0, V1110 -> 1622 (512+1110), V1232 -> 1744, V1262 -> 1774
        registers_[0] = 131;       // AI0
        registers_[512 + 1110] = 131;
        registers_[512 + 1100] = 0;
        registers_[512 + 1230] = 36;
        registers_[512 + 1232] = 37;
        registers_[512 + 1260] = 1597;
        registers_[512 + 1262] = 1594;
        registers_[512 + 50] = 1200;
        registers_[512 + 51] = 1250;
        registers_[512 + 52] = 0;
        registers_[512 + 53] = 0;
        registers_[512 + 1300] = 2820;
        registers_[512 + 1310] = 360;
    }

private:
    core::ConnectionConfig config_;
    mutable std::mutex mutex_;
    bool connected_ = false;
    bool fail_connection_ = false;
    core::Milliseconds simulated_latency_{0};
    std::optional<ExceptionCode> injected_exception_;
    std::map<core::RegisterAddress, std::int16_t> registers_;
};

} // namespace plc::modbus
