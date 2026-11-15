#include "test_framework.hpp"
#include "plc/modbus/modbus_pdu.hpp"
#include "plc/core/endian.hpp"

using namespace plc;
using namespace plc::modbus;

TEST_CASE(ModbusPduTest, BuildReadHoldingRegistersRequest) {
    auto req = ModbusPdu::build_read_holding_registers_request(0x1234, 1, 1110, 4);
    ASSERT_EQ(req.size(), 12);

    // MBAP TxID (0x1234)
    ASSERT_EQ(req[0], 0x12);
    ASSERT_EQ(req[1], 0x34);

    // Protocol ID (0x0000)
    ASSERT_EQ(req[2], 0x00);
    ASSERT_EQ(req[3], 0x00);

    // Length (0x0006)
    ASSERT_EQ(req[4], 0x00);
    ASSERT_EQ(req[5], 0x06);

    // Unit ID (1)
    ASSERT_EQ(req[6], 0x01);

    // FC 0x03
    ASSERT_EQ(req[7], 0x03);

    // Start Address (1110 = 0x0456)
    ASSERT_EQ(req[8], 0x04);
    ASSERT_EQ(req[9], 0x56);

    // Count (4 = 0x0004)
    ASSERT_EQ(req[10], 0x00);
    ASSERT_EQ(req[11], 0x04);
}

TEST_CASE(ModbusPduTest, ParseMbapHeaderSuccess) {
    std::vector<core::Byte> buf = {0x00, 0x0A, 0x00, 0x00, 0x00, 0x05, 0x01};
    auto res = ModbusPdu::parse_mbap_header(buf.data(), buf.size());
    ASSERT_TRUE(res.ok());
    ASSERT_EQ(res.value().transaction_id, 10);
    ASSERT_EQ(res.value().protocol_id, 0);
    ASSERT_EQ(res.value().length, 5);
    ASSERT_EQ(res.value().unit_id, 1);
}

TEST_CASE(ModbusPduTest, ParseMbapHeaderShortBuffer) {
    std::vector<core::Byte> buf = {0x00, 0x01, 0x00, 0x00};
    auto res = ModbusPdu::parse_mbap_header(buf.data(), buf.size());
    ASSERT_FALSE(res.ok());
}

TEST_CASE(ModbusPduTest, ParsePayloadSignedIntegers) {
    // 3 registers: 131, -14 (0xFFF2), 0
    std::vector<core::Byte> payload = {
        0x00, 0x83, // 131
        0xFF, 0xF2, // -14
        0x00, 0x00  // 0
    };

    auto res = ModbusPdu::parse_read_holding_registers_payload(payload.data(), payload.size(), 3);
    ASSERT_TRUE(res.ok());
    const auto& regs = res.value();
    ASSERT_EQ(regs.size(), 3);
    ASSERT_EQ(regs[0], 131);
    ASSERT_EQ(regs[1], -14);
    ASSERT_EQ(regs[2], 0);
}

TEST_CASE(ModbusPduTest, ExceptionResponseDetection) {
    ASSERT_TRUE(ModbusPdu::is_exception_response(0x83));
    ASSERT_FALSE(ModbusPdu::is_exception_response(0x03));

    auto exc = ModbusPdu::parse_exception_code(0x02);
    ASSERT_EQ(static_cast<int>(exc), static_cast<int>(ExceptionCode::IllegalDataAddress));
}
