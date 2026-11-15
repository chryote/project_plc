#include "plc/app/config.hpp"
#include "plc/app/runner.hpp"

#include <iostream>

int main(int argc, char* argv[]) {
    try {
        auto config = plc::app::AppConfig::parse(argc, argv);
        plc::app::Runner runner(config);
        return runner.run();
    } catch (const std::exception& ex) {
        std::cerr << "Fatal Exception: " << ex.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "Unknown Fatal Exception\n";
        return 1;
    }
}
