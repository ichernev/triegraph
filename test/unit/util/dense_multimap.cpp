#include "util/util.h"
#include "util/dense_multimap.h"

// #include <assert.h>
// #include <iostream>
#include "testlib/test.h"

#include <vector>
#include <utility>
#include <algorithm>

using namespace triegraph;

using DM = DenseMultimap<u32, u32>;

int m = test::define_module(__FILE__, [] {

test::define_test("empty", [] {
    auto dm = DM();

    assert(!dm.contains(u32(0)));
    assert(!dm.contains(u32(1)));
    assert(dm.keys().empty());
    assert(dm.values_for(0).empty());
});

test::define_test("keys", [] {
    auto dm = DM(std::vector<std::pair<u32, u32>> {
        { 0, 5 }, { 1, 6 }, { 5, 8 },
    });

    dm.sanity_check();
    assert(std::ranges::equal(dm.keys(), std::vector<u32> { 0, 1, 5 }));
});

test::define_test("pair_iter", [] {
    auto v = std::vector<std::pair<u32, u32>> {
        { 0, 5 }, { 1, 6 }, {1, 7}, { 5, 8 },
    };
    auto dm = DM(v);

    dm.sanity_check();
    assert(std::ranges::equal(dm, v));
});

test::define_test("values_for", [] {
    auto dm = DM(std::vector<std::pair<u32, u32>> {
        { 0, 5 }, { 1, 6 }, {1, 7}, { 5, 8 },
    });

    dm.sanity_check();
    assert(std::ranges::equal(dm.values_for(0), std::vector<u32> { 5 }));
    assert(std::ranges::equal(dm.values_for(1), std::vector<u32> { 6, 7 }));
    assert(std::ranges::equal(dm.values_for(2), std::vector<u32> { }));
    assert(std::ranges::equal(dm.values_for(3), std::vector<u32> { }));
    assert(std::ranges::equal(dm.values_for(4), std::vector<u32> { }));
    assert(std::ranges::equal(dm.values_for(5), std::vector<u32> { 8 }));
    assert(std::ranges::equal(dm.values_for(6), std::vector<u32> { }));
});

test::define_test("contains", [] {
    auto dm = DM(std::vector<std::pair<u32, u32>> {
        { 0, 5 }, { 1, 6 }, {1, 7}, { 5, 8 },
    });
    dm.sanity_check();
    assert( dm.contains(0));
    assert( dm.contains(1));
    assert(!dm.contains(2));
    assert(!dm.contains(3));
    assert(!dm.contains(4));
    assert( dm.contains(5));
    assert(!dm.contains(6));
});

});
