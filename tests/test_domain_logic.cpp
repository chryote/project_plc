#include "test_framework.hpp"
#include "plc/domain/side_slip.hpp"
#include "plc/domain/axle_load.hpp"
#include "plc/domain/speedometer.hpp"

using namespace plc::domain;

TEST_CASE(DomainLogicTest, SideSlipScalingStandard) {
    SideSlipCalibration calib{90, 176, 0, 320};
    ASSERT_TRUE(calib.is_valid());

    double at_low = SideSlipCalculator::calculate_scaled(90, calib);
    ASSERT_NEAR(at_low, 0.0, 0.001);

    double at_high = SideSlipCalculator::calculate_scaled(176, calib);
    ASSERT_NEAR(at_high, 320.0, 0.001);

    double at_mid = SideSlipCalculator::calculate_scaled(133, calib);
    ASSERT_NEAR(at_mid, 160.0, 0.1);
}

TEST_CASE(DomainLogicTest, SideSlipScalingDivideByZeroProtection) {
    SideSlipCalibration calib{100, 100, 0, 50}; // raw_high == raw_low
    ASSERT_FALSE(calib.is_valid());

    double res = SideSlipCalculator::calculate_scaled(120, calib);
    ASSERT_EQ(res, 0.0); // Safely returns disp_low
}

TEST_CASE(DomainLogicTest, AxleLoadCalculations) {
    AxleLoadReading reading;
    reading.ch1_left.live_weight = 500;
    reading.ch1_left.avg_weight = 495;
    reading.ch2_right.live_weight = 550;
    reading.ch2_right.avg_weight = 545;

    reading.sumbu_weights = {1050, 1100, 0, 0};

    ASSERT_EQ(reading.total_live_weight(), 1050);
    ASSERT_EQ(reading.total_avg_weight(), 1040);
    ASSERT_EQ(reading.total_gross_vehicle_weight(), 2150);

    // Difference: |500 - 550| / 550 * 100% = 9.0909%
    ASSERT_NEAR(reading.side_difference_percent(), 9.0909, 0.01);
}

TEST_CASE(DomainLogicTest, SpeedometerCalculations) {
    SpeedometerReading reading;
    reading.encoder_ppr = 360;
    reading.pulse_per_minute = 3600; // 10 RPM
    reading.display_speed = 10;

    ASSERT_NEAR(reading.calculated_rpm(), 10.0, 0.001);

    // Roller Ø 0.35m => Circumference = pi * 0.35 = 1.099557 m
    // Distance / min = 10 * 1.099557 = 10.99557 m/min
    // km/h = (10.99557 * 60) / 1000 = 0.6597 km/h
    ASSERT_NEAR(reading.estimated_kmh(0.35), 0.6597, 0.01);
}
