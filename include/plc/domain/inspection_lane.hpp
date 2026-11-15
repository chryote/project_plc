#pragma once

#include "plc/core/result.hpp"
#include "plc/modbus/modbus_client.hpp"
#include "plc/domain/haiwell_mapping.hpp"
#include "plc/domain/side_slip.hpp"
#include "plc/domain/axle_load.hpp"
#include "plc/domain/speedometer.hpp"

#include <chrono>
#include <map>

namespace plc::domain {

struct InspectionLaneSnapshot {
    std::chrono::system_clock::time_point timestamp;
    SideSlipReading side_slip{};
    AxleLoadReading axle_load{};
    SpeedometerReading speedometer{};
    bool success = false;
};

struct ProbeEntry {
    core::RegisterAddress address = 0;
    std::string symbol;
    std::int16_t snap1 = 0;
    std::int16_t snap2 = 0;
    bool is_fluctuating = false;
};

class InspectionLaneService {
public:
    explicit InspectionLaneService(modbus::IModbusClient& client,
                                  AddressingMode mode = AddressingMode::DirectZeroBased);

    core::Result<SideSlipReading> read_side_slip();
    core::Result<AxleLoadReading> read_axle_load();
    core::Result<SpeedometerReading> read_speedometer();
    core::Result<InspectionLaneSnapshot> read_full_snapshot();

    // Probe range of registers (like blind_probe)
    core::Result<std::vector<ProbeEntry>> probe_range(
        core::RegisterAddress start,
        core::RegisterCount count,
        core::Milliseconds delay_between_snaps = core::Milliseconds(500));

    void set_addressing_mode(AddressingMode mode) noexcept {
        addressing_mode_ = mode;
    }

    [[nodiscard]] AddressingMode addressing_mode() const noexcept {
        return addressing_mode_;
    }

private:
    [[nodiscard]] core::RegisterAddress map_addr(core::RegisterAddress direct_v) const noexcept;

    modbus::IModbusClient& client_;
    AddressingMode addressing_mode_;
};

} // namespace plc::domain
