#include "plc/domain/haiwell_mapping.hpp"
#include <charconv>
#include <cctype>
#include <stdexcept>

namespace plc::domain {

bool HaiwellMapping::is_valid_symbol(std::string_view symbol) noexcept {
    if (symbol.empty()) return false;
    char prefix = static_cast<char>(std::toupper(symbol[0]));
    if (prefix == 'V') {
        if (symbol.size() < 2) return false;
        int num = 0;
        auto [ptr, ec] = std::from_chars(symbol.data() + 1, symbol.data() + symbol.size(), num);
        return ec == std::errc() && ptr == (symbol.data() + symbol.size()) && num >= 0 && num <= 65535;
    }
    if ((prefix == 'A') && symbol.size() >= 3) {
        char type = static_cast<char>(std::toupper(symbol[1]));
        if (type == 'I' || type == 'Q') {
            int num = 0;
            auto [ptr, ec] = std::from_chars(symbol.data() + 2, symbol.data() + symbol.size(), num);
            return ec == std::errc() && ptr == (symbol.data() + symbol.size()) && num >= 0 && num <= 255;
        }
    }
    return false;
}

core::RegisterAddress HaiwellMapping::to_modbus_address(std::string_view symbol, AddressingMode mode) {
    if (symbol.empty()) {
        throw std::invalid_argument("Empty register symbol");
    }

    char p0 = static_cast<char>(std::toupper(symbol[0]));
    if (p0 == 'V') {
        int num = 0;
        auto [ptr, ec] = std::from_chars(symbol.data() + 1, symbol.data() + symbol.size(), num);
        if (ec != std::errc() || ptr != (symbol.data() + symbol.size())) {
            throw std::invalid_argument("Invalid V-register format: " + std::string(symbol));
        }
        if (mode == AddressingMode::HaiwellOffset) {
            return static_cast<core::RegisterAddress>(512 + num);
        }
        return static_cast<core::RegisterAddress>(num);
    }

    if (p0 == 'A' && symbol.size() >= 3) {
        char p1 = static_cast<char>(std::toupper(symbol[1]));
        int num = 0;
        auto [ptr, ec] = std::from_chars(symbol.data() + 2, symbol.data() + symbol.size(), num);
        if (ec != std::errc() || ptr != (symbol.data() + symbol.size())) {
            throw std::invalid_argument("Invalid AI/AQ register format: " + std::string(symbol));
        }

        if (p1 == 'I') { // AI0..255 -> 0..255
            return static_cast<core::RegisterAddress>(num);
        } else if (p1 == 'Q') { // AQ0..255 -> 256..511
            return static_cast<core::RegisterAddress>(256 + num);
        }
    }

    // Attempt numeric fallback
    int num = 0;
    auto [ptr, ec] = std::from_chars(symbol.data(), symbol.data() + symbol.size(), num);
    if (ec == std::errc() && ptr == (symbol.data() + symbol.size())) {
        return static_cast<core::RegisterAddress>(num);
    }

    throw std::invalid_argument("Unrecognized symbol: " + std::string(symbol));
}

std::string HaiwellMapping::to_symbol(core::RegisterAddress addr, AddressingMode mode) {
    if (mode == AddressingMode::HaiwellOffset) {
        if (addr < 256) {
            return "AI" + std::to_string(addr);
        }
        if (addr < 512) {
            return "AQ" + std::to_string(addr - 256);
        }
        return "V" + std::to_string(addr - 512);
    }
    return "V" + std::to_string(addr);
}

} // namespace plc::domain
