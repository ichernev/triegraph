#include <assert.h>
#include <iostream>

#include <string>
#include <vector>
#include <functional>

#include "util/logger.h"

std::string test_module_name;
std::vector<std::pair<std::string, std::function<void()>>> tests;

namespace test {
    static int define_module(const char *filename, std::function<void()> &&fn) {
        test_module_name = filename;
        fn();
        return 0;
    }

    static void define_test(const char *test, std::function<void()> &&fn) {
        tests.emplace_back(test, std::move(fn));
    }
}


int main(int argc, char *argv[]) {
    triegraph::Logger::disable();

    std::cerr << "TEST MODULE " << test_module_name << std::endl;
    for (const auto &test : tests) {
        std::cerr << "  TEST " << test.first << " ";
        test.second();
        std::cerr << "[OK]" << std::endl;
    }

    return 0;
}
