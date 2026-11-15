#pragma once

#include "plc/core/types.hpp"
#include <array>
#include <cstdint>
#include <cmath>

namespace plc::domain {

struct WeighingChannel {
    std::int16_t live_weight = 0;
    std::int16_t avg_weight = 0;
};

struct AxleLoadReading {
    WeighingChannel ch1_left{};  // Slot 2 CH1 (V1232, V1230)
    WeighingChannel ch2_right{}; // Slot 2 CH2 (V1262, V1260)

    // Stored Axle Weights (Sumbu 1 to 4 from V50..V53)
    std::array<std::int16_t, 4> sumbu_weights{0, 0, 0, 0};

    [[nodiscard]] std::int32_t total_live_weight() const noexcept {
        return static_cast<std::int32_t>(ch1_left.live_weight) + ch2_right.live_weight;
    }

    [[nodiscard]] std::int32_t total_avg_weight() const noexcept {
        return static_cast<std::int32_t>(ch1_left.avg_weight) + ch2_right.avg_weight;
    }

    [[nodiscard]] std::int32_t total_gross_vehicle_weight() const noexcept {
        std::int32_t total = 0;
        for (auto w : sumbu_weights) {
            if (w > 0) total += w;
        }
        return total;
    }

    // Dynamic brake or load balance difference percentage
    [[nodiscard]] double side_difference_percent() const noexcept {
        const double w1 = std::abs(static_cast<double>(ch1_left.live_weight));
        const double w2 = std::abs(static_cast<double>(ch2_right.live_weight));
        const double max_val = std::max(w1, w2);
        if (max_val <= 0.001) return 0.0;
        return (std::abs(w1 - w2) / max_val) * 100.0;
    }
};

} // namespace plc::domain
