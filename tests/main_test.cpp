#include "test_framework.hpp"

int main() {
    return plc::test::TestRegistry::instance().run_all();
}
