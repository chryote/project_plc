#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <cmath>
#include <sstream>

namespace plc::test {

struct TestCase {
    std::string suite;
    std::string name;
    std::function<void()> func;
};

class TestRegistry {
public:
    static TestRegistry& instance() {
        static TestRegistry reg;
        return reg;
    }

    void add_test(std::string suite, std::string name, std::function<void()> func) {
        tests_.push_back({std::move(suite), std::move(name), std::move(func)});
    }

    int run_all() {
        int passed = 0;
        int failed = 0;

        std::cout << "\n============================================================\n"
                  << "                 RUNNING UNIT TEST SUITE                    \n"
                  << "============================================================\n\n";

        for (const auto& test : tests_) {
            std::cout << "[ RUN      ] " << test.suite << "." << test.name << "\n";
            current_test_failed_ = false;
            current_failure_msg_.clear();

            try {
                test.func();
            } catch (const std::exception& e) {
                current_test_failed_ = true;
                current_failure_msg_ = "Uncaught exception: " + std::string(e.what());
            } catch (...) {
                current_test_failed_ = true;
                current_failure_msg_ = "Uncaught non-standard exception";
            }

            if (!current_test_failed_) {
                std::cout << "[       OK ] " << test.suite << "." << test.name << "\n";
                ++passed;
            } else {
                std::cout << "[  FAILED  ] " << test.suite << "." << test.name << "\n"
                          << "             " << current_failure_msg_ << "\n";
                ++failed;
            }
        }

        std::cout << "\n------------------------------------------------------------\n"
                  << "Test Results: " << passed << " passed, " << failed << " failed, "
                  << tests_.size() << " total.\n"
                  << "============================================================\n";

        return failed == 0 ? 0 : 1;
    }

    void record_failure(const std::string& msg) {
        current_test_failed_ = true;
        if (!current_failure_msg_.empty()) {
            current_failure_msg_ += "\n             ";
        }
        current_failure_msg_ += msg;
    }

    [[nodiscard]] bool current_failed() const noexcept {
        return current_test_failed_;
    }

private:
    std::vector<TestCase> tests_;
    bool current_test_failed_ = false;
    std::string current_failure_msg_;
};

struct AutoRegister {
    AutoRegister(std::string suite, std::string name, std::function<void()> func) {
        TestRegistry::instance().add_test(std::move(suite), std::move(name), std::move(func));
    }
};

} // namespace plc::test

#define TEST_CASE(suite_name, test_name) \
    static void test_##suite_name##_##test_name(); \
    static ::plc::test::AutoRegister reg_##suite_name##_##test_name(#suite_name, #test_name, test_##suite_name##_##test_name); \
    static void test_##suite_name##_##test_name()

#define ASSERT_TRUE(condition) do { \
    if (!(condition)) { \
        ::plc::test::TestRegistry::instance().record_failure("Assertion failed: " #condition " at " __FILE__ ":" + std::to_string(__LINE__)); \
        return; \
    } \
} while(0)

#define ASSERT_FALSE(condition) do { \
    if (condition) { \
        ::plc::test::TestRegistry::instance().record_failure("Assertion failed: !(" #condition ") at " __FILE__ ":" + std::to_string(__LINE__)); \
        return; \
    } \
} while(0)

#define ASSERT_EQ(actual, expected) do { \
    auto _act = (actual); \
    auto _exp = (expected); \
    if (!(_act == _exp)) { \
        std::ostringstream _ss; \
        _ss << "Assertion failed: " #actual " == " #expected " (got " << _act << ", expected " << _exp << ") at " __FILE__ ":" << __LINE__; \
        ::plc::test::TestRegistry::instance().record_failure(_ss.str()); \
        return; \
    } \
} while(0)

#define ASSERT_NEAR(actual, expected, tolerance) do { \
    double _act = static_cast<double>(actual); \
    double _exp = static_cast<double>(expected); \
    double _diff = std::abs(_act - _exp); \
    if (_diff > (tolerance)) { \
        std::ostringstream _ss; \
        _ss << "Assertion failed: " #actual " near " #expected " within " #tolerance " (diff " << _diff << ") at " __FILE__ ":" << __LINE__; \
        ::plc::test::TestRegistry::instance().record_failure(_ss.str()); \
        return; \
    } \
} while(0)
