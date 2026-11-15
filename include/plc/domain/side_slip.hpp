#pragma once

#include "plc/core/types.hpp"
#include <cstdint>
#include <string>

namespace plc::domain {

struct SideSlipCalibration {
    std::int16_t raw_low = 130;
    std::int16_t raw_high = 30;
    std::int16_t disp_low = -1;
    std::int16_t disp_high = 30;

    [[nodiscard]] bool is_valid() const noexcept {
        return raw_high != raw_low;
    }
};

struct SideSlipReading {
    std::int16_t raw_adc = 0;
    std::int16_t plc_scaled = 0;
    double calculated_scaled = 0.0;
    SideSlipCalibration calibration{};

    [[nodiscard]] double deviation() const noexcept {
        return static_cast<double>(plc_scaled) - calculated_scaled;
    }
};

class SideSlipCalculator {
public:
    [[nodiscard]] static double calculate_scaled(
        std::int16_t raw,
        const SideSlipCalibration& calib) noexcept;
};

} // namespace plc::domain
