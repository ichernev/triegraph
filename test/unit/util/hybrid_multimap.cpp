// SPDX-License-Identifier: MPL-2.0
/*
 * Copyright (c) 2021, Iskren Chernev
 */

#include "util/hybrid_multimap.h"
#include "util/util.h"

#include "testlib/test.h"

#include <unordered_map>
#include <utility>
#include <vector>

using namespace triegraph;
using HMM = HybridMultimap<u32, u32, u32,
      std::unordered_map<u32, std::pair<u32, u32>>>;

int m = test::define_module(__FILE__, [] {
test::define_test("empty", [] {
    auto hmm = HMM();

    assert(test::equal_sorted(hmm, std::vector<std::pair<u32, u32>> {}));
    assert(test::equal_sorted(hmm.keys(), std::vector<u32> {}));
    assert(test::equal_sorted(hmm.values_for(42), std::vector<u32> {}));
});

test::define_test("few elems", [] {
    auto pairs = std::vector<std::pair<u32, u32>> {
        { 0, 5 },
        { 0, 6 },
        { 1, 3 },
        { 1, 10 },
        { 20, 5 },
        { 31, 30 }
    };
    auto hmm = HMM(pairs);
    assert(test::equal_sorted(hmm, pairs));
    assert(test::equal_sorted(hmm.keys(), std::vector<u32> { 0, 1, 20, 31 }));
    assert(test::equal_sorted(hmm.values_for(0), std::vector<u32> { 5, 6 }));
    assert(test::equal_sorted(hmm.values_for(1), std::vector<u32> { 3, 10 }));
    assert(test::equal_sorted(hmm.values_for(20), std::vector<u32> { 5 }));
    assert(test::equal_sorted(hmm.values_for(31), std::vector<u32> { 30 }));
    assert(test::equal_sorted(hmm.values_for(32), std::vector<u32> { }));
});

test::define_test("single key", [] {
    auto pairs = std::vector<std::pair<u32, u32>> {
        { 0, 5 },
        { 0, 6 },
    };
    auto hmm = HMM(pairs);
    assert(test::equal_sorted(hmm, pairs));
    assert(test::equal_sorted(hmm.keys(), std::vector<u32> { 0 }));
    assert(test::equal_sorted(hmm.values_for(0), std::vector<u32> { 5, 6 }));
});
});
