#include "plc/app/formatter.hpp"
#include "plc/domain/haiwell_mapping.hpp"

#include <sstream>
#include <iomanip>

namespace plc::app {

std::string Formatter::format_lane_text(const domain::InspectionLaneSnapshot& snap) {
    std::ostringstream ss;
    ss << "============================================================\n"
       << "             VEHICLE INSPECTION LANE SNAPSHOT               \n"
       << "============================================================\n";

    ss << "\n[1. SIDE SLIP TESTER]\n"
       << "  - Raw ADC (V1110):        " << snap.side_slip.raw_adc << "\n"
       << "  - PLC Display (V1100):    " << snap.side_slip.plc_scaled << " mm/m\n"
       << "  - Tool Calculated:        " << std::fixed << std::setprecision(2) << snap.side_slip.calculated_scaled << " mm/m\n"
       << "  - Deviation:              " << snap.side_slip.deviation() << " mm/m\n"
       << "  - Calibration (V1102..5): [" << snap.side_slip.calibration.raw_low << ", "
       << snap.side_slip.calibration.raw_high << ", "
       << snap.side_slip.calibration.disp_low << ", "
       << snap.side_slip.calibration.disp_high << "]\n";

    ss << "\n[2. AXLE LOAD / WEIGHING & BRAKE FORCE (Slot 2)]\n"
       << "  - Left / CH1 Live (V1232):" << std::setw(6) << snap.axle_load.ch1_left.live_weight
       << " kg  (Avg V1230: " << snap.axle_load.ch1_left.avg_weight << " kg)\n"
       << "  - Right/ CH2 Live (V1262):" << std::setw(6) << snap.axle_load.ch2_right.live_weight
       << " kg  (Avg V1260: " << snap.axle_load.ch2_right.avg_weight << " kg)\n"
       << "  - Total Live Axle Weight: " << std::setw(6) << snap.axle_load.total_live_weight() << " kg\n"
       << "  - Left/Right Difference:  " << std::fixed << std::setprecision(1)
       << snap.axle_load.side_difference_percent() << " %\n"
       << "  - Stored Axles (Sumbu 1..4):\n"
       << "      * Sumbu 1 (V50):      " << snap.axle_load.sumbu_weights[0] << " kg\n"
       << "      * Sumbu 2 (V51):      " << snap.axle_load.sumbu_weights[1] << " kg\n"
       << "      * Sumbu 3 (V52):      " << snap.axle_load.sumbu_weights[2] << " kg\n"
       << "      * Sumbu 4 (V53):      " << snap.axle_load.sumbu_weights[3] << " kg\n"
       << "      * Total Gross Weight: " << snap.axle_load.total_gross_vehicle_weight() << " kg\n";

    ss << "\n[3. SPEEDOMETER TESTER]\n"
       << "  - Display Speed (V1300):  " << snap.speedometer.display_speed << "\n"
       << "  - Encoder PPR (V1310):    " << snap.speedometer.encoder_ppr << " PPR\n"
       << "  - Raw Pulses (V1320):     " << snap.speedometer.raw_pulses << "\n"
       << "  - Pulse/Min (V1322):      " << snap.speedometer.pulse_per_minute << "\n"
       << "  - Calculated Roller RPM:  " << std::fixed << std::setprecision(1)
       << snap.speedometer.calculated_rpm() << " RPM\n"
       << "  - Estimated Vehicle Speed:" << std::fixed << std::setprecision(1)
       << snap.speedometer.estimated_kmh() << " km/h (roller Ø350mm)\n";

    ss << "============================================================\n";
    return ss.str();
}

std::string Formatter::format_lane_json(const domain::InspectionLaneSnapshot& snap) {
    std::ostringstream ss;
    ss << "{\n"
       << "  \"side_slip\": {\n"
       << "    \"raw_adc\": " << snap.side_slip.raw_adc << ",\n"
       << "    \"plc_scaled\": " << snap.side_slip.plc_scaled << ",\n"
       << "    \"calculated_scaled\": " << snap.side_slip.calculated_scaled << ",\n"
       << "    \"deviation\": " << snap.side_slip.deviation() << ",\n"
       << "    \"calibration\": {\n"
       << "      \"raw_low\": " << snap.side_slip.calibration.raw_low << ",\n"
       << "      \"raw_high\": " << snap.side_slip.calibration.raw_high << ",\n"
       << "      \"disp_low\": " << snap.side_slip.calibration.disp_low << ",\n"
       << "      \"disp_high\": " << snap.side_slip.calibration.disp_high << "\n"
       << "    }\n"
       << "  },\n"
       << "  \"axle_load\": {\n"
       << "    \"ch1_left\": {\"live\": " << snap.axle_load.ch1_left.live_weight
       << ", \"avg\": " << snap.axle_load.ch1_left.avg_weight << "},\n"
       << "    \"ch2_right\": {\"live\": " << snap.axle_load.ch2_right.live_weight
       << ", \"avg\": " << snap.axle_load.ch2_right.avg_weight << "},\n"
       << "    \"total_live_weight\": " << snap.axle_load.total_live_weight() << ",\n"
       << "    \"difference_percent\": " << snap.axle_load.side_difference_percent() << ",\n"
       << "    \"sumbu\": ["
       << snap.axle_load.sumbu_weights[0] << ", "
       << snap.axle_load.sumbu_weights[1] << ", "
       << snap.axle_load.sumbu_weights[2] << ", "
       << snap.axle_load.sumbu_weights[3] << "],\n"
       << "    \"gross_vehicle_weight\": " << snap.axle_load.total_gross_vehicle_weight() << "\n"
       << "  },\n"
       << "  \"speedometer\": {\n"
       << "    \"display_speed\": " << snap.speedometer.display_speed << ",\n"
       << "    \"encoder_ppr\": " << snap.speedometer.encoder_ppr << ",\n"
       << "    \"raw_pulses\": " << snap.speedometer.raw_pulses << ",\n"
       << "    \"pulse_per_minute\": " << snap.speedometer.pulse_per_minute << ",\n"
       << "    \"calculated_rpm\": " << snap.speedometer.calculated_rpm() << ",\n"
       << "    \"estimated_kmh\": " << snap.speedometer.estimated_kmh() << "\n"
       << "  }\n"
       << "}\n";
    return ss.str();
}

std::string Formatter::format_lane_csv(const domain::InspectionLaneSnapshot& snap) {
    std::ostringstream ss;
    ss << "side_slip_raw,side_slip_disp,ch1_live,ch2_live,total_axle,sumbu1,sumbu2,sumbu3,sumbu4,speed_display,speed_kmh\n";
    ss << snap.side_slip.raw_adc << ","
       << snap.side_slip.plc_scaled << ","
       << snap.axle_load.ch1_left.live_weight << ","
       << snap.axle_load.ch2_right.live_weight << ","
       << snap.axle_load.total_live_weight() << ","
       << snap.axle_load.sumbu_weights[0] << ","
       << snap.axle_load.sumbu_weights[1] << ","
       << snap.axle_load.sumbu_weights[2] << ","
       << snap.axle_load.sumbu_weights[3] << ","
       << snap.speedometer.display_speed << ","
       << snap.speedometer.estimated_kmh() << "\n";
    return ss.str();
}

std::string Formatter::format_registers_text(
    core::RegisterAddress start_addr,
    const std::vector<std::int16_t>& vals,
    domain::AddressingMode mode)
{
    std::ostringstream ss;
    ss << std::left << std::setw(12) << "Symbol"
       << std::setw(10) << "Modbus(0)"
       << std::setw(12) << "Dec (Int16)"
       << std::setw(10) << "Hex (Word)"
       << "\n";
    ss << std::string(44, '-') << "\n";

    for (std::size_t i = 0; i < vals.size(); ++i) {
        core::RegisterAddress addr = static_cast<core::RegisterAddress>(start_addr + i);
        std::string symbol = domain::HaiwellMapping::to_symbol(addr, mode);
        std::uint16_t uval = static_cast<std::uint16_t>(vals[i]);

        ss << std::left << std::setw(12) << symbol
           << std::right << std::setw(8) << addr << "  "
           << std::setw(10) << vals[i] << "  "
           << "0x" << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << uval
           << std::dec << std::setfill(' ')
           << "\n";
    }
    return ss.str();
}

std::string Formatter::format_registers_json(
    core::RegisterAddress start_addr,
    const std::vector<std::int16_t>& vals,
    domain::AddressingMode mode)
{
    std::ostringstream ss;
    ss << "[\n";
    for (std::size_t i = 0; i < vals.size(); ++i) {
        core::RegisterAddress addr = static_cast<core::RegisterAddress>(start_addr + i);
        std::string symbol = domain::HaiwellMapping::to_symbol(addr, mode);
        ss << "  {\"address\": " << addr
           << ", \"symbol\": \"" << symbol << "\""
           << ", \"value\": " << vals[i]
           << "}" << (i + 1 < vals.size() ? "," : "") << "\n";
    }
    ss << "]\n";
    return ss.str();
}

std::string Formatter::format_probe_text(const std::vector<domain::ProbeEntry>& entries) {
    std::ostringstream ss;
    ss << std::left << std::setw(12) << "Symbol"
       << std::setw(10) << "Address"
       << std::setw(10) << "Snap 1"
       << std::setw(10) << "Snap 2"
       << "Status\n";
    ss << std::string(55, '-') << "\n";

    for (const auto& e : entries) {
        ss << std::left << std::setw(12) << e.symbol
           << std::setw(10) << e.address
           << std::setw(10) << e.snap1
           << std::setw(10) << e.snap2
           << (e.is_fluctuating ? "<<< LIVE / FLUCTUATING!" : "Static Non-Zero")
           << "\n";
    }

    if (entries.empty()) {
        ss << "(No non-zero or active registers found in scanned range)\n";
    }
    return ss.str();
}

std::string Formatter::format_probe_json(const std::vector<domain::ProbeEntry>& entries) {
    std::ostringstream ss;
    ss << "[\n";
    for (std::size_t i = 0; i < entries.size(); ++i) {
        const auto& e = entries[i];
        ss << "  {\"symbol\": \"" << e.symbol << "\""
           << ", \"address\": " << e.address
           << ", \"snap1\": " << e.snap1
           << ", \"snap2\": " << e.snap2
           << ", \"fluctuating\": " << (e.is_fluctuating ? "true" : "false")
           << "}" << (i + 1 < entries.size() ? "," : "") << "\n";
    }
    ss << "]\n";
    return ss.str();
}

} // namespace plc::app
