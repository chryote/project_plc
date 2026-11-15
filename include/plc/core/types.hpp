#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <chrono>

namespace plc::core {

using Byte = std::uint8_t;
using Word = std::uint16_t;
using DWord = std::uint32_t;

using RegisterAddress = std::uint16_t;
using RegisterCount = std::uint16_t;
using UnitId = std::uint8_t;
using TransactionId = std::uint16_t;

using Milliseconds = std::chrono::milliseconds;

struct ConnectionConfig {
    std::string host = "192.168.70.111";
    std::uint16_t port = 502;
    UnitId unit_id = 1;
    Milliseconds timeout = Milliseconds(3000);
    bool keep_alive = true;
};

} // namespace plc::core
