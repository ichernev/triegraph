#include <assert.h>
#include <iostream>

#include <string>
#include <vector>
#include <functional>

#include "util/logger.h"

std::string test_module_name;
std::vector<std::pair<std::string, std::function<void()>>> tests;

namespace test {

    template <std::ranges::input_range R,
             typename T = std::ranges::range_value_t<R>,
             typename Cmp = std::less<T>>
    static std::vector<T> sorted(R &&range) {
        std::vector<T> res;
        std::ranges::copy(range, std::back_inserter(res));
        std::ranges::sort(res, Cmp {});
        return res;
    }

    static bool equal_sorted(
            std::ranges::input_range auto&& range1,
            std::ranges::input_range auto&& range2) {
        return std::ranges::equal(sorted(range1), range2);
    }

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
