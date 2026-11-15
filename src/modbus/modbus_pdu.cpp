#include "plc/modbus/modbus_pdu.hpp"
#include "plc/core/endian.hpp"

#include <cstring>

namespace plc::modbus {

std::vector<core::Byte> ModbusPdu::build_read_holding_registers_request(
    core::TransactionId tx_id,
    core::UnitId unit_id,
    core::RegisterAddress start_addr,
    core::RegisterCount count)
{
    std::vector<core::Byte> request(12);

    const core::Word be_tx_id = core::host_to_be16(tx_id);
    const core::Word be_proto = core::host_to_be16(0);
    const core::Word be_len   = core::host_to_be16(6); // 1 byte unit_id + 1 byte FC + 2 bytes addr + 2 bytes count
    const core::Word be_addr  = core::host_to_be16(start_addr);
    const core::Word be_count = core::host_to_be16(count);

    std::memcpy(&request[0], &be_tx_id, 2);
    std::memcpy(&request[2], &be_proto, 2);
    std::memcpy(&request[4], &be_len, 2);
    request[6] = unit_id;
    request[7] = static_cast<core::Byte>(FunctionCode::ReadHoldingRegisters);
    std::memcpy(&request[8], &be_addr, 2);
    std::memcpy(&request[10], &be_count, 2);

    return request;
}

core::Result<MbapHeader> ModbusPdu::parse_mbap_header(const core::Byte* buffer, std::size_t size) {
    if (!buffer || size < MBAP_HEADER_SIZE) {
        return core::Error{core::StatusCode::InvalidPdu, "Buffer too short for MBAP header"};
    }

    core::Word raw_tx_id, raw_proto, raw_len;
    std::memcpy(&raw_tx_id, &buffer[0], 2);
    std::memcpy(&raw_proto, &buffer[2], 2);
    std::memcpy(&raw_len, &buffer[4], 2);

    MbapHeader header;
    header.transaction_id = core::be16_to_host(raw_tx_id);
    header.protocol_id    = core::be16_to_host(raw_proto);
    header.length         = core::be16_to_host(raw_len);
    header.unit_id        = buffer[6];

    if (header.protocol_id != 0) {
        return core::Error{core::StatusCode::InvalidPdu, "Invalid Modbus TCP protocol identifier"};
    }

    return header;
}

core::Result<std::vector<std::int16_t>> ModbusPdu::parse_read_holding_registers_payload(
    const core::Byte* data,
    std::size_t byte_count,
    core::RegisterCount expected_count)
{
    if (!data) {
        return core::Error{core::StatusCode::InvalidPdu, "Null payload buffer"};
    }

    const std::size_t expected_bytes = static_cast<std::size_t>(expected_count) * 2;
    if (byte_count != expected_bytes) {
        return core::Error{core::StatusCode::InvalidPdu, 
            "Byte count mismatch: expected " + std::to_string(expected_bytes) + 
            ", got " + std::to_string(byte_count)};
    }

    std::vector<std::int16_t> registers;
    registers.reserve(expected_count);

    for (std::size_t i = 0; i < expected_bytes; i += 2) {
        core::Word raw_word;
        std::memcpy(&raw_word, &data[i], 2);
        registers.push_back(core::be16_to_host_signed(raw_word));
    }

    return registers;
}

bool ModbusPdu::is_exception_response(core::Byte function_code) noexcept {
    return (function_code & 0x80) != 0;
}

ExceptionCode ModbusPdu::parse_exception_code(core::Byte raw_code) noexcept {
    return static_cast<ExceptionCode>(raw_code);
}

} // namespace plc::modbus
