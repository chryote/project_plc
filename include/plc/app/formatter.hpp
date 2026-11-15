#pragma once

#include "plc/domain/inspection_lane.hpp"
#include <string>
#include <vector>

namespace plc::app {

class Formatter {
public:
    static std::string format_lane_text(const domain::InspectionLaneSnapshot& snap);
    static std::string format_lane_json(const domain::InspectionLaneSnapshot& snap);
    static std::string format_lane_csv(const domain::InspectionLaneSnapshot& snap);

    static std::string format_registers_text(
        core::RegisterAddress start_addr,
        const std::vector<std::int16_t>& vals,
        domain::AddressingMode mode);

    static std::string format_registers_json(
        core::RegisterAddress start_addr,
        const std::vector<std::int16_t>& vals,
        domain::AddressingMode mode);

    static std::string format_probe_text(const std::vector<domain::ProbeEntry>& entries);
    static std::string format_probe_json(const std::vector<domain::ProbeEntry>& entries);
};

} // namespace plc::app
