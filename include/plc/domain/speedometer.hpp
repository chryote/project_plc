#pragma once

#include "plc/core/types.hpp"
#include <cstdint>

namespace plc::domain {

struct SpeedometerReading {
    std::int32_t raw_pulses = 0;         // V1320 (32-bit)
    std::int32_t pulse_per_minute = 0;   // V1322 (32-bit)
    std::int16_t encoder_ppr = 360;      // V1310 (PPR)
    std::int32_t display_speed = 0;      // V1300 (32-bit)

    [[nodiscard]] double calculated_rpm() const noexcept {
        if (encoder_ppr <= 0) return 0.0;
        return static_cast<double>(pulse_per_minute) / static_cast<double>(encoder_ppr);
    }

    [[nodiscard]] double estimated_kmh(double roller_diameter_meters = 0.35) const noexcept {
        const double rpm = calculated_rpm();
        constexpr double pi = 3.14159265358979323846;
        const double circumference_m = pi * roller_diameter_meters;
        const double meters_per_minute = rpm * circumference_m;
        return (meters_per_minute * 60.0) / 1000.0; // km/h
    }
};

} // namespace plc::domain
