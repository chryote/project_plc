#pragma once

#include "plc/core/types.hpp"
#include "plc/core/result.hpp"
#include "plc/modbus/modbus_defs.hpp"

#include <vector>
#include <array>

namespace plc::modbus {

struct MbapHeader {
    core::TransactionId transaction_id = 0;
    core::Word protocol_id = 0; // Always 0 for Modbus TCP
    core::Word length = 0;      // Remaining bytes including unit_id + PDU
    core::UnitId unit_id = 1;
};

class ModbusPdu {
public:
    static std::vector<core::Byte> build_read_holding_registers_request(
        core::TransactionId tx_id,
        core::UnitId unit_id,
        core::RegisterAddress start_addr,
        core::RegisterCount count);

    static core::Result<MbapHeader> parse_mbap_header(const core::Byte* buffer, std::size_t size);

    static core::Result<std::vector<std::int16_t>> parse_read_holding_registers_payload(
        const core::Byte* data,
        std::size_t byte_count,
        core::RegisterCount expected_count);

    static bool is_exception_response(core::Byte function_code) noexcept;
    static ExceptionCode parse_exception_code(core::Byte raw_code) noexcept;
};

} // namespace plc::modbus
