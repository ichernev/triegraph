// SPDX-License-Identifier: MPL-2.0
/*
 * Copyright (c) 2021, Iskren Chernev
 */

#include "testlib/test.h"

#include "triegraph/util/util.h"
#include "triegraph/util/vector_pairs.h"
#include "triegraph/util/compact_vector.h"

using namespace triegraph;

static void test_non_empty(auto &&vp) {
    using VP = std::decay_t<decltype(vp)>;

    assert(vp.size() == 0);
    vp.sort_by_fwd();
    vp.sort_by_rev();
    vp.unique();
    auto fwd = vp.fwd_pairs();
    assert(fwd.begin() == fwd.end());
    assert(fwd.empty());
    assert(fwd.size() == 0);
    auto rev = vp.rev_pairs();
    assert(rev.begin() == rev.end());
    // assert(rev.empty());
    assert(rev.size() == 0);

    vp.emplace_back(1, 2);
    vp.emplace_back(1, 1);
    vp.emplace_back(0, 3);
    vp.emplace_back(0, 5);
    vp.emplace_back(0, 3);
    assert(std::ranges::equal(vp.fwd_pairs(), typename VP::fwd_vec {
                {1, 2}, {1, 1}, {0, 3}, {0, 5}, {0, 3}}));
    assert(std::ranges::equal(vp.rev_pairs(), typename VP::rev_vec {
                {2, 1}, {1, 1}, {3, 0}, {5, 0}, {3, 0}}));
    vp.sort_by_fwd();
    assert(std::ranges::equal(vp.fwd_pairs(), typename VP::fwd_vec {
                {0, 3}, {0, 3}, {0, 5}, {1, 1}, {1, 2}}));
    vp.sort_by_rev();
    assert(std::ranges::equal(vp.rev_pairs(), typename VP::rev_vec {
                {1, 1}, {2, 1}, {3, 0}, {3, 0}, {5, 0}}));
    vp.unique();
    assert(vp.size() == 4u);
    assert(std::ranges::equal(vp.rev_pairs(), typename VP::rev_vec {
                {1, 1}, {2, 1}, {3, 0}, {5, 0}}));
}

int m = test::define_module(__FILE__, [] {

test::define_test("Empty impl", [] {
    using VP = VectorPairsEmpty<u32, u32>;
    auto vp = VP();

    assert(vp.size() == 0);
    vp.sort_by_fwd();
    vp.sort_by_rev();
    vp.unique();
    auto fwd = vp.fwd_pairs();
    assert(fwd.begin() == fwd.end());
    assert(fwd.empty());
    assert(fwd.size() == 0);
    auto rev = vp.rev_pairs();
    assert(rev.begin() == rev.end());
    assert(rev.empty());
    assert(rev.size() == 0);
});

test::define_test("Simple impl", [] {
    test_non_empty(VectorPairsSimple<u32, u32>());
});

test::define_test("Dual impl", [] {
    test_non_empty(VectorPairsDual<u32, u32>());
});

test::define_test("Dual take", [] {
    auto vp = VectorPairsDual<u32, u32>();
    vp.emplace_back(0, 5);
    vp.emplace_back(2, 6);
    assert(vp.size() == 2u);

    auto v1 = vp.take_v1();
    auto v2 = vp.take_v2();
    assert(vp.size() == 0u);
    assert(std::ranges::equal(v1, std::vector<u32> { 0, 2 }));
    assert(std::ranges::equal(v2, std::vector<u32> { 5, 6 }));
});

test::define_test("Dual with CompactVector", [] {
    using VP = VectorPairsDual<u32, u32, CompactVector<u32>, CompactVector<u32>>;
    auto vp = VP {};

    compact_vector_set_bits(vp.get_v1(), 10);
    compact_vector_set_bits(vp.get_v2(), 10);

    vp.emplace_back(0, 1);
    vp.emplace_back(1, 0);
    vp.emplace_back(4, 3);
    vp.emplace_back(0, 0);
    vp.emplace_back(1, 0); // dupe
    vp.emplace_back(1, 3);

    vp.sort_by_fwd().unique();
    assert(std::ranges::equal(vp.fwd_pairs(), VP::fwd_vec {
                {0, 0}, {0, 1}, {1, 0}, {1, 3}, {4, 3} }));
    vp.sort_by_rev();
    assert(std::ranges::equal(vp.rev_pairs(), VP::rev_vec {
                {0, 0}, {0, 1}, {1, 0}, {3, 1}, {3, 4} }));
});

});
