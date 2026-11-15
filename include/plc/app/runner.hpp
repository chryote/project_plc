#pragma once

#include "plc/app/config.hpp"
#include <memory>

namespace plc::app {

class Runner {
public:
    explicit Runner(AppConfig config);
    int run();

private:
    int execute_lane();
    int execute_read();
    int execute_probe();
    int execute_benchmark();

    AppConfig config_;
};

} // namespace plc::app
