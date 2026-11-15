#include "test_framework.hpp"
#include "plc/modbus/modbus_mock.hpp"
#include "plc/domain/inspection_lane.hpp"

using namespace plc;
using namespace plc::modbus;
using namespace plc::domain;

TEST_CASE(MockClientTest, BasicReadWrite) {
    MockModbusClient client;
    ASSERT_FALSE(client.is_connected());

    auto conn_res = client.connect();
    ASSERT_TRUE(conn_res.ok());
    ASSERT_TRUE(client.is_connected());

    client.set_register(100, 42);
    auto read_res = client.read_holding_registers(100, 1);
    ASSERT_TRUE(read_res.ok());
    ASSERT_EQ(read_res.value().size(), 1);
    ASSERT_EQ(read_res.value()[0], 42);

    client.disconnect();
    ASSERT_FALSE(client.is_connected());
}

TEST_CASE(MockClientTest, InjectedException) {
    MockModbusClient client;
    client.connect();

    client.inject_exception(ExceptionCode::IllegalDataAddress);
    auto read_res = client.read_holding_registers(100, 1);
    ASSERT_FALSE(read_res.ok());
    ASSERT_EQ(static_cast<int>(read_res.error().code), static_cast<int>(core::StatusCode::ModbusException));

    client.inject_exception(std::nullopt);
    auto ok_res = client.read_holding_registers(100, 1);
    ASSERT_TRUE(ok_res.ok());
}

TEST_CASE(MockClientTest, InspectionLaneServiceIntegration) {
    MockModbusClient client;
    client.connect();

    InspectionLaneService service(client, AddressingMode::DirectZeroBased);

    // Read full snapshot with default mock values
    auto snap_res = service.read_full_snapshot();
    ASSERT_TRUE(snap_res.ok());

    const auto& snap = snap_res.value();
    ASSERT_TRUE(snap.success);

    // Check Side Slip
    ASSERT_EQ(snap.side_slip.raw_adc, 131);
    ASSERT_EQ(snap.side_slip.plc_scaled, 0);

    // Check Axle Load
    ASSERT_EQ(snap.axle_load.ch1_left.avg_weight, 36);
    ASSERT_EQ(snap.axle_load.ch1_left.live_weight, 37);
    ASSERT_EQ(snap.axle_load.ch2_right.avg_weight, 1597);
    ASSERT_EQ(snap.axle_load.ch2_right.live_weight, 1594);
    ASSERT_EQ(snap.axle_load.sumbu_weights[0], 1200);
    ASSERT_EQ(snap.axle_load.sumbu_weights[1], 1250);

    // Check Speedometer
    ASSERT_EQ(snap.speedometer.display_speed, 2820);
    ASSERT_EQ(snap.speedometer.encoder_ppr, 360);
}

TEST_CASE(MockClientTest, ProbeRangeDetection) {
    MockModbusClient client;
    client.connect();

    InspectionLaneService service(client, AddressingMode::DirectZeroBased);

    auto probe_res = service.probe_range(1100, 20, core::Milliseconds(10));
    ASSERT_TRUE(probe_res.ok());

    // Should find V1100, V1102, V1103, V1105, V1110
    const auto& entries = probe_res.value();
    ASSERT_TRUE(entries.size() > 0);

    bool found_raw = false;
    for (const auto& e : entries) {
        if (e.address == 1110) {
            found_raw = true;
            ASSERT_EQ(e.snap1, 131);
        }
    }
    ASSERT_TRUE(found_raw);
}
