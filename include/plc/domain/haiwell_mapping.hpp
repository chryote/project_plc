#pragma once

#include "plc/core/types.hpp"
#include <string>
#include <optional>

namespace plc::domain {

enum class AddressingMode {
    DirectZeroBased,   // V<n> = n (e.g., V50 = 50, V1110 = 1110)
    HaiwellOffset      // AI0..255 = 0..255, AQ0..255 = 256..511, V<n> = 512 + n
};

class HaiwellMapping {
public:
    static core::RegisterAddress to_modbus_address(
        std::string_view symbol,
        AddressingMode mode = AddressingMode::DirectZeroBased);

    static std::string to_symbol(
        core::RegisterAddress addr,
        AddressingMode mode = AddressingMode::DirectZeroBased);

    static bool is_valid_symbol(std::string_view symbol) noexcept;
};

// Known Well-Defined Register Offsets for Sorong Vehicle Inspection Lane
namespace registers {
    // Direct register numbers (n)
    constexpr core::RegisterAddress V_SIDE_SLIP_RAW      = 1110;
    constexpr core::RegisterAddress V_SIDE_SLIP_SCALED   = 1100;
    constexpr core::RegisterAddress V_SIDE_SLIP_RAW_LOW  = 1102;
    constexpr core::RegisterAddress V_SIDE_SLIP_RAW_HIGH = 1103;
    constexpr core::RegisterAddress V_SIDE_SLIP_DISP_LOW = 1104;
    constexpr core::RegisterAddress V_SIDE_SLIP_DISP_HIGH= 1105;

    constexpr core::RegisterAddress V_CH1_AVG_WEIGHT     = 1230;
    constexpr core::RegisterAddress V_CH1_LIVE_WEIGHT    = 1232;
    constexpr core::RegisterAddress V_CH2_AVG_WEIGHT     = 1260;
    constexpr core::RegisterAddress V_CH2_LIVE_WEIGHT    = 1262;

    constexpr core::RegisterAddress V_SUMBU_1            = 50;
    constexpr core::RegisterAddress V_SUMBU_2            = 51;
    constexpr core::RegisterAddress V_SUMBU_3            = 52;
    constexpr core::RegisterAddress V_SUMBU_4            = 53;

    constexpr core::RegisterAddress V_SPEED_DISPLAY      = 1300;
    constexpr core::RegisterAddress V_SPEED_ENCODER_PPR  = 1310;
    constexpr core::RegisterAddress V_SPEED_RAW_PULSES   = 1320;
    constexpr core::RegisterAddress V_SPEED_PULSE_PER_MIN= 1322;
}

} // namespace plc::domain
