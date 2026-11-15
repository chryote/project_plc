#include "plc/domain/side_slip.hpp"

namespace plc::domain {

double SideSlipCalculator::calculate_scaled(
    std::int16_t raw,
    const SideSlipCalibration& calib) noexcept
{
    if (calib.raw_high == calib.raw_low) {
        return static_cast<double>(calib.disp_low);
    }

    const double delta_raw = static_cast<double>(calib.raw_high - calib.raw_low);
    const double delta_disp = static_cast<double>(calib.disp_high - calib.disp_low);
    const double raw_diff = static_cast<double>(raw - calib.raw_low);

    return static_cast<double>(calib.disp_low) + (raw_diff * delta_disp) / delta_raw;
}

} // namespace plc::domain
