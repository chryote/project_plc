#pragma once

#include <cstdint>
#include <cstring>
#include <bit>

namespace plc::core {

[[nodiscard]] constexpr std::uint16_t byteswap16(std::uint16_t value) noexcept {
    return static_cast<std::uint16_t>((value << 8) | (value >> 8));
}

[[nodiscard]] constexpr std::uint32_t byteswap32(std::uint32_t value) noexcept {
    return ((value & 0x000000FFU) << 24) |
           ((value & 0x0000FF00U) << 8)  |
           ((value & 0x00FF0000U) >> 8)  |
           ((value & 0xFF000000U) >> 24);
}

[[nodiscard]] inline std::uint16_t host_to_be16(std::uint16_t val) noexcept {
    if constexpr (std::endian::native == std::endian::little) {
        return byteswap16(val);
    } else {
        return val;
    }
}

[[nodiscard]] inline std::uint16_t be16_to_host(std::uint16_t val) noexcept {
    if constexpr (std::endian::native == std::endian::little) {
        return byteswap16(val);
    } else {
        return val;
    }
}

[[nodiscard]] inline std::int16_t be16_to_host_signed(std::uint16_t val) noexcept {
    const std::uint16_t host = be16_to_host(val);
    std::int16_t res;
    std::memcpy(&res, &host, sizeof(res));
    return res;
}

[[nodiscard]] inline std::uint32_t host_to_be32(std::uint32_t val) noexcept {
    if constexpr (std::endian::native == std::endian::little) {
        return byteswap32(val);
    } else {
        return val;
    }
}

[[nodiscard]] inline std::uint32_t be32_to_host(std::uint32_t val) noexcept {
    if constexpr (std::endian::native == std::endian::little) {
        return byteswap32(val);
    } else {
        return val;
    }
}

[[nodiscard]] inline std::int32_t be32_to_host_signed(std::uint32_t val) noexcept {
    const std::uint32_t host = be32_to_host(val);
    std::int32_t res;
    std::memcpy(&res, &host, sizeof(res));
    return res;
}

// Convert two 16-bit registers (Modbus big-endian high word + low word or low + high) to 32-bit
[[nodiscard]] inline std::int32_t words_to_int32(std::uint16_t word0, std::uint16_t word1, bool little_word_first = true) noexcept {
    std::uint32_t u32 = 0;
    if (little_word_first) {
        u32 = (static_cast<std::uint32_t>(word1) << 16) | word0;
    } else {
        u32 = (static_cast<std::uint32_t>(word0) << 16) | word1;
    }
    std::int32_t result;
    std::memcpy(&result, &u32, sizeof(result));
    return result;
}

[[nodiscard]] inline float words_to_float(std::uint16_t word0, std::uint16_t word1, bool little_word_first = true) noexcept {
    std::uint32_t u32 = 0;
    if (little_word_first) {
        u32 = (static_cast<std::uint32_t>(word1) << 16) | word0;
    } else {
        u32 = (static_cast<std::uint32_t>(word0) << 16) | word1;
    }
    float result;
    std::memcpy(&result, &u32, sizeof(result));
    return result;
}

} // namespace plc::core
