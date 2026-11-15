#pragma once

#include <cstdint>
#include <string>

namespace plc::modbus {

enum class FunctionCode : std::uint8_t {
    ReadCoils = 0x01,
    ReadDiscreteInputs = 0x02,
    ReadHoldingRegisters = 0x03,
    ReadInputRegisters = 0x04,
    WriteSingleCoil = 0x05,
    WriteSingleRegister = 0x06,
    WriteMultipleCoils = 0x0F,
    WriteMultipleRegisters = 0x10
};

enum class ExceptionCode : std::uint8_t {
    IllegalFunction = 0x01,
    IllegalDataAddress = 0x02,
    IllegalDataValue = 0x03,
    ServerDeviceFailure = 0x04,
    Acknowledge = 0x05,
    ServerDeviceBusy = 0x06,
    NegativeAcknowledge = 0x07,
    MemoryParityError = 0x08,
    GatewayPathUnavailable = 0x0A,
    GatewayTargetDeviceFailed = 0x0B
};

[[nodiscard]] inline std::string exception_to_string(ExceptionCode code) {
    switch (code) {
        case ExceptionCode::IllegalFunction: return "Illegal Function (0x01)";
        case ExceptionCode::IllegalDataAddress: return "Illegal Data Address (0x02)";
        case ExceptionCode::IllegalDataValue: return "Illegal Data Value (0x03)";
        case ExceptionCode::ServerDeviceFailure: return "Server Device Failure (0x04)";
        case ExceptionCode::Acknowledge: return "Acknowledge (0x05)";
        case ExceptionCode::ServerDeviceBusy: return "Server Device Busy (0x06)";
        case ExceptionCode::NegativeAcknowledge: return "Negative Acknowledge (0x07)";
        case ExceptionCode::MemoryParityError: return "Memory Parity Error (0x08)";
        case ExceptionCode::GatewayPathUnavailable: return "Gateway Path Unavailable (0x0A)";
        case ExceptionCode::GatewayTargetDeviceFailed: return "Gateway Target Device Failed (0x0B)";
        default: return "Unknown Modbus Exception (" + std::to_string(static_cast<int>(code)) + ")";
    }
}

constexpr std::size_t MBAP_HEADER_SIZE = 7;
constexpr std::size_t MAX_MODBUS_TCP_FRAME_SIZE = 260;
constexpr std::size_t MAX_REGISTERS_PER_READ = 125;

} // namespace plc::modbus
