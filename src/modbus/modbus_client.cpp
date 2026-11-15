#include "plc/modbus/modbus_client.hpp"
#include "plc/modbus/modbus_pdu.hpp"
#include "plc/core/logger.hpp"

#include <array>

namespace plc::modbus {

ModbusTcpClient::ModbusTcpClient(core::ConnectionConfig config,
                                 std::unique_ptr<network::ISocket> socket)
    : config_(std::move(config)),
      socket_(socket ? std::move(socket) : std::make_unique<network::TcpSocket>()) {}

ModbusTcpClient::~ModbusTcpClient() {
    disconnect();
}

ModbusTcpClient::ModbusTcpClient(ModbusTcpClient&& other) noexcept
    : config_(std::move(other.config_)),
      socket_(std::move(other.socket_)),
      next_tx_id_(other.next_tx_id_.load()) {}

ModbusTcpClient& ModbusTcpClient::operator=(ModbusTcpClient&& other) noexcept {
    if (this != &other) {
        config_ = std::move(other.config_);
        socket_ = std::move(other.socket_);
        next_tx_id_.store(other.next_tx_id_.load());
    }
    return *this;
}

core::Result<void> ModbusTcpClient::connect() {
    if (is_connected()) {
        return {};
    }
    LOG_DEBUG("Connecting to PLC at " << config_.host << ":" << config_.port << "...");
    return socket_->connect(config_.host, config_.port, config_.timeout);
}

void ModbusTcpClient::disconnect() noexcept {
    if (socket_) {
        socket_->close();
    }
}

bool ModbusTcpClient::is_connected() const noexcept {
    return socket_ && socket_->is_connected();
}

core::Result<std::vector<std::int16_t>> ModbusTcpClient::read_holding_registers(
    core::RegisterAddress start,
    core::RegisterCount count)
{
    if (count == 0 || count > MAX_REGISTERS_PER_READ) {
        return core::Error{core::StatusCode::InvalidParameter,
            "Invalid register count: " + std::to_string(count) + " (must be 1.." + std::to_string(MAX_REGISTERS_PER_READ) + ")"};
    }

    if (!is_connected()) {
        auto conn_res = connect();
        if (!conn_res) {
            return conn_res.error();
        }
    }

    const core::TransactionId tx_id = next_tx_id_.fetch_add(1, std::memory_order_relaxed);
    auto request = ModbusPdu::build_read_holding_registers_request(tx_id, config_.unit_id, start, count);

    auto send_res = socket_->send_all(request.data(), request.size());
    if (!send_res) {
        return send_res.error();
    }

    // Read MBAP header + FC + ByteCount/ErrorCode (9 bytes)
    std::array<core::Byte, 9> header_buf{};
    auto recv_hdr = socket_->receive_exact(header_buf.data(), header_buf.size());
    if (!recv_hdr) {
        return recv_hdr.error();
    }

    auto mbap_res = ModbusPdu::parse_mbap_header(header_buf.data(), MBAP_HEADER_SIZE);
    if (!mbap_res) {
        return mbap_res.error();
    }

    const auto& mbap = mbap_res.value();
    if (mbap.transaction_id != tx_id) {
        return core::Error{core::StatusCode::TransactionMismatch,
            "Transaction ID mismatch: expected " + std::to_string(tx_id) + ", got " + std::to_string(mbap.transaction_id)};
    }

    const core::Byte resp_fc = header_buf[7];
    const core::Byte param_byte = header_buf[8];

    // Check for Modbus Exception
    if (ModbusPdu::is_exception_response(resp_fc)) {
        auto exc_code = ModbusPdu::parse_exception_code(param_byte);
        return core::Error{core::StatusCode::ModbusException,
            "Modbus Exception from PLC: " + exception_to_string(exc_code)};
    }

    if (resp_fc != static_cast<core::Byte>(FunctionCode::ReadHoldingRegisters)) {
        return core::Error{core::StatusCode::InvalidPdu,
            "Unexpected Function Code in response: " + std::to_string(resp_fc)};
    }

    const std::size_t byte_count = param_byte;
    const std::size_t expected_bytes = static_cast<std::size_t>(count) * 2;
    if (byte_count != expected_bytes) {
        return core::Error{core::StatusCode::InvalidPdu,
            "Modbus payload length mismatch: expected " + std::to_string(expected_bytes) + ", got " + std::to_string(byte_count)};
    }

    // Receive the register payload
    std::vector<core::Byte> payload(byte_count);
    auto recv_payload = socket_->receive_exact(payload.data(), byte_count);
    if (!recv_payload) {
        return recv_payload.error();
    }

    return ModbusPdu::parse_read_holding_registers_payload(payload.data(), byte_count, count);
}

} // namespace plc::modbus
