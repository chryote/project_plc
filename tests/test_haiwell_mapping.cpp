#include "test_framework.hpp"
#include "plc/domain/haiwell_mapping.hpp"

using namespace plc::domain;

TEST_CASE(HaiwellMappingTest, DirectAddressing) {
    ASSERT_EQ(HaiwellMapping::to_modbus_address("V50", AddressingMode::DirectZeroBased), 50);
    ASSERT_EQ(HaiwellMapping::to_modbus_address("V1110", AddressingMode::DirectZeroBased), 1110);
    ASSERT_EQ(HaiwellMapping::to_modbus_address("v1232", AddressingMode::DirectZeroBased), 1232);
    ASSERT_EQ(HaiwellMapping::to_symbol(1110, AddressingMode::DirectZeroBased), "V1110");
}

TEST_CASE(HaiwellMappingTest, OffsetAddressing) {
    // V<n> -> 512 + n
    ASSERT_EQ(HaiwellMapping::to_modbus_address("V50", AddressingMode::HaiwellOffset), 562);
    ASSERT_EQ(HaiwellMapping::to_modbus_address("V1110", AddressingMode::HaiwellOffset), 1622);

    // AI0..255 -> 0..255
    ASSERT_EQ(HaiwellMapping::to_modbus_address("AI0", AddressingMode::HaiwellOffset), 0);
    ASSERT_EQ(HaiwellMapping::to_modbus_address("AI10", AddressingMode::HaiwellOffset), 10);

    // AQ0..255 -> 256..511
    ASSERT_EQ(HaiwellMapping::to_modbus_address("AQ0", AddressingMode::HaiwellOffset), 256);
    ASSERT_EQ(HaiwellMapping::to_modbus_address("AQ1", AddressingMode::HaiwellOffset), 257);

    // Reverse to_symbol
    ASSERT_EQ(HaiwellMapping::to_symbol(0, AddressingMode::HaiwellOffset), "AI0");
    ASSERT_EQ(HaiwellMapping::to_symbol(256, AddressingMode::HaiwellOffset), "AQ0");
    ASSERT_EQ(HaiwellMapping::to_symbol(1622, AddressingMode::HaiwellOffset), "V1110");
}

TEST_CASE(HaiwellMappingTest, SymbolValidation) {
    ASSERT_TRUE(HaiwellMapping::is_valid_symbol("V0"));
    ASSERT_TRUE(HaiwellMapping::is_valid_symbol("V1110"));
    ASSERT_TRUE(HaiwellMapping::is_valid_symbol("AI0"));
    ASSERT_TRUE(HaiwellMapping::is_valid_symbol("AQ255"));

    ASSERT_FALSE(HaiwellMapping::is_valid_symbol(""));
    ASSERT_FALSE(HaiwellMapping::is_valid_symbol("INVALID"));
    ASSERT_FALSE(HaiwellMapping::is_valid_symbol("V"));
    ASSERT_FALSE(HaiwellMapping::is_valid_symbol("AI"));
}
