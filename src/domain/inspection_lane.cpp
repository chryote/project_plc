#include "plc/domain/inspection_lane.hpp"
#include "plc/core/endian.hpp"
#include "plc/core/logger.hpp"

#include <thread>
#include <algorithm>

namespace plc::domain {

InspectionLaneService::InspectionLaneService(modbus::IModbusClient& client, AddressingMode mode)
    : client_(client), addressing_mode_(mode) {}

core::RegisterAddress InspectionLaneService::map_addr(core::RegisterAddress direct_v) const noexcept {
    if (addressing_mode_ == AddressingMode::HaiwellOffset) {
        return static_cast<core::RegisterAddress>(512 + direct_v);
    }
    return direct_v;
}

core::Result<SideSlipReading> InspectionLaneService::read_side_slip() {
    SideSlipReading reading{};

    // Read V1100..V1105 (6 registers: V1100=Display, V1101, V1102=RawLow, V1103=RawHigh, V1104=DispLow, V1105=DispHigh)
    auto regs_res = client_.read_holding_registers(map_addr(registers::V_SIDE_SLIP_SCALED), 6);
    if (!regs_res) {
        return regs_res.error();
    }
    const auto& regs = regs_res.value();
    reading.plc_scaled = regs[0];
    reading.calibration.raw_low = regs[2];
    reading.calibration.raw_high = regs[3];
    reading.calibration.disp_low = regs[4];
    reading.calibration.disp_high = regs[5];

    // Read V1110 (Raw ADC)
    auto raw_res = client_.read_holding_registers(map_addr(registers::V_SIDE_SLIP_RAW), 1);
    if (!raw_res) {
        return raw_res.error();
    }
    reading.raw_adc = raw_res.value()[0];

    reading.calculated_scaled = SideSlipCalculator::calculate_scaled(reading.raw_adc, reading.calibration);
    return reading;
}

core::Result<AxleLoadReading> InspectionLaneService::read_axle_load() {
    AxleLoadReading reading{};

    // Read CH1 (V1230..V1232: V1230=Avg, V1231=Reserved, V1232=Live)
    auto ch1_res = client_.read_holding_registers(map_addr(registers::V_CH1_AVG_WEIGHT), 3);
    if (!ch1_res) {
        return ch1_res.error();
    }
    reading.ch1_left.avg_weight = ch1_res.value()[0];
    reading.ch1_left.live_weight = ch1_res.value()[2];

    // Read CH2 (V1260..V1262: V1260=Avg, V1261=Reserved, V1262=Live)
    auto ch2_res = client_.read_holding_registers(map_addr(registers::V_CH2_AVG_WEIGHT), 3);
    if (!ch2_res) {
        return ch2_res.error();
    }
    reading.ch2_right.avg_weight = ch2_res.value()[0];
    reading.ch2_right.live_weight = ch2_res.value()[2];

    // Read Sumbu 1..4 (V50..V53)
    auto sumbu_res = client_.read_holding_registers(map_addr(registers::V_SUMBU_1), 4);
    if (!sumbu_res) {
        return sumbu_res.error();
    }
    for (std::size_t i = 0; i < 4; ++i) {
        reading.sumbu_weights[i] = sumbu_res.value()[i];
    }

    return reading;
}

core::Result<SpeedometerReading> InspectionLaneService::read_speedometer() {
    SpeedometerReading reading{};

    // Read V1300..V1301 (Display speed 32-bit)
    auto spd_res = client_.read_holding_registers(map_addr(registers::V_SPEED_DISPLAY), 2);
    if (spd_res) {
        reading.display_speed = core::words_to_int32(
            static_cast<std::uint16_t>(spd_res.value()[0]),
            static_cast<std::uint16_t>(spd_res.value()[1]),
            true);
    }

    // Read V1310 (PPR)
    auto ppr_res = client_.read_holding_registers(map_addr(registers::V_SPEED_ENCODER_PPR), 1);
    if (ppr_res) {
        reading.encoder_ppr = ppr_res.value()[0];
    }

    // Read V1320..V1323 (V1320..1321 = Raw Pulses 32-bit, V1322..1323 = Pulse per minute 32-bit)
    auto pulses_res = client_.read_holding_registers(map_addr(registers::V_SPEED_RAW_PULSES), 4);
    if (pulses_res) {
        const auto& p = pulses_res.value();
        reading.raw_pulses = core::words_to_int32(
            static_cast<std::uint16_t>(p[0]),
            static_cast<std::uint16_t>(p[1]),
            true);
        reading.pulse_per_minute = core::words_to_int32(
            static_cast<std::uint16_t>(p[2]),
            static_cast<std::uint16_t>(p[3]),
            true);
    }

    return reading;
}

core::Result<InspectionLaneSnapshot> InspectionLaneService::read_full_snapshot() {
    InspectionLaneSnapshot snap;
    snap.timestamp = std::chrono::system_clock::now();

    auto ss_res = read_side_slip();
    if (!ss_res) return ss_res.error();
    snap.side_slip = ss_res.value();

    auto ax_res = read_axle_load();
    if (!ax_res) return ax_res.error();
    snap.axle_load = ax_res.value();

    auto sp_res = read_speedometer();
    if (!sp_res) return sp_res.error();
    snap.speedometer = sp_res.value();

    snap.success = true;
    return snap;
}

core::Result<std::vector<ProbeEntry>> InspectionLaneService::probe_range(
    core::RegisterAddress start,
    core::RegisterCount count,
    core::Milliseconds delay_between_snaps)
{
    constexpr core::RegisterCount BATCH_SIZE = 100;
    std::map<core::RegisterAddress, std::int16_t> snap1;
    std::map<core::RegisterAddress, std::int16_t> snap2;

    // Snapshot 1
    for (core::RegisterCount offset = 0; offset < count; offset += BATCH_SIZE) {
        core::RegisterCount current_batch = std::min(BATCH_SIZE, static_cast<core::RegisterCount>(count - offset));
        auto res = client_.read_holding_registers(start + offset, current_batch);
        if (res) {
            for (core::RegisterCount i = 0; i < current_batch; ++i) {
                snap1[start + offset + i] = res.value()[i];
            }
        }
    }

    if (delay_between_snaps.count() > 0) {
        std::this_thread::sleep_for(delay_between_snaps);
    }

    // Snapshot 2
    for (core::RegisterCount offset = 0; offset < count; offset += BATCH_SIZE) {
        core::RegisterCount current_batch = std::min(BATCH_SIZE, static_cast<core::RegisterCount>(count - offset));
        auto res = client_.read_holding_registers(start + offset, current_batch);
        if (res) {
            for (core::RegisterCount i = 0; i < current_batch; ++i) {
                snap2[start + offset + i] = res.value()[i];
            }
        }
    }

    std::vector<ProbeEntry> results;
    for (const auto& [addr, val1] : snap1) {
        std::int16_t val2 = val1;
        auto it = snap2.find(addr);
        if (it != snap2.end()) {
            val2 = it->second;
        }

        // Only include if non-zero or fluctuating
        if (val1 != 0 || val2 != 0) {
            ProbeEntry entry;
            entry.address = addr;
            entry.symbol = HaiwellMapping::to_symbol(addr, addressing_mode_);
            entry.snap1 = val1;
            entry.snap2 = val2;
            entry.is_fluctuating = (val1 != val2);
            results.push_back(entry);
        }
    }

    return results;
}

} // namespace plc::domain
