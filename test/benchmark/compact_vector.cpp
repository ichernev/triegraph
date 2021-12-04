// SPDX-License-Identifier: MPL-2.0
/*
 * Copyright (c) 2021, Iskren Chernev
 */

#include "util/util.h"
#include "util/compact_vector.h"

#include "testlib/test.h"

#include <algorithm>
#include <random>
#include <ranges>

using namespace triegraph;
using V32 = std::vector<u32>;
using CV32 = triegraph::CompactVector<u32>;
using V64 = std::vector<u64>;
using CV64 = triegraph::CompactVector<u64>;

enum TestType { FILL, ITER_IDX, ITER_IT, SORT, SHUFFLE };
std::string tt_str(TestType tt) {
    switch (tt) {
        case FILL:
            return "fill";
        case ITER_IDX:
            return "iter-idx";
        case ITER_IT:
            return "iter-it";
        case SORT:
            return "sort";
        case SHUFFLE:
            return "shuffle";
        default:
            return "unknown";
    }
}

template <typename T>
std::string type_explain_(const std::vector<T> &v) { return "vector"; }
template <typename T>
std::string type_explain_(const CompactVector<T> &v) { return "CompactVector"; }
template <typename T>
std::string type_explain() {
    return type_explain_(T{}) + std::to_string(sizeof(typename T::value_type));
}

template <typename CV, u32 bits>
struct CompactVectorTester : public test::TestCaseBase {

    CompactVectorTester(TestType test, u64 size, u64 checksum = 0)
        : TestCaseBase(type_explain<CV>() + "::" + tt_str(test) + "::" + std::to_string(size)),
          test(test),
          size(size),
          checksum(checksum)
    {}

    void prepare() {
        _init();
        if (test == FILL) return;
        _fill();
        if (test == ITER_IT || test == ITER_IDX || test == SHUFFLE) return;
        _shuffle();
    }

    void run() {
        switch (test) {
            case FILL:
                _fill();
                break;
            case ITER_IT:
                for (u32 i = 0; i < 100; ++i) {
                    // std::cerr << _iter_it() << std::endl;
                    assert(_iter_it() == checksum);
                }
                break;
            case ITER_IDX:
                for (u32 i = 0; i < 100; ++i) {
                    // std::cerr << _iter_idx() << std::endl;
                    assert(_iter_idx() == checksum);
                }
                break;
            case SORT:
                _sort();
                break;
            case SHUFFLE:
                _shuffle();
                break;
        }
    }

    void _init() { compact_vector_set_bits(cv, bits); }
    void _fill() { std::ranges::copy(std::ranges::iota_view {u64(0), size}, std::back_inserter(cv)); }
    void _shuffle() { std::shuffle(cv.begin(), cv.end(), std::mt19937 {}); }
    void _sort() { std::sort(cv.begin(), cv.end()); }
    u64 _iter_it() { return std::accumulate(cv.begin(), cv.end(), 0ul); }
    u64 _iter_idx() {
        u64 sum = 0;
        for (u64 i = 0; i < cv.size(); ++i) {
            sum += cv[i];
        }
        return sum;
    }

    CV cv;
    TestType test;
    u64 size;
    u64 checksum;
};

int m = test::define_module(__FILE__, [] {
    auto _m = 1000 * 1000;
    test::add_test<CompactVectorTester<CV64, 31>>(FILL, 10 * _m);
    test::add_test<CompactVectorTester<CV64, 31>>(ITER_IT, 10 * _m, 49999995000000ul);
    test::add_test<CompactVectorTester<CV64, 31>>(ITER_IDX, 10 * _m, 49999995000000ul);
    test::add_test<CompactVectorTester<CV64, 31>>(SORT, 10 * _m);
    test::add_test<CompactVectorTester<CV64, 31>>(SHUFFLE, 10 * _m);

    test::add_test<CompactVectorTester<V64, 31>>(FILL, 10 * _m);
    test::add_test<CompactVectorTester<V64, 31>>(ITER_IT, 10 * _m, 49999995000000ul);
    test::add_test<CompactVectorTester<V64, 31>>(ITER_IDX, 10 * _m, 49999995000000ul);
    test::add_test<CompactVectorTester<V64, 31>>(SORT, 10 * _m);
    test::add_test<CompactVectorTester<V64, 31>>(SHUFFLE, 10 * _m);
});
