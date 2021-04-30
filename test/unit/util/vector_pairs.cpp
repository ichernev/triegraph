#include "testlib/test.h"

#include "util/util.h"
#include "util/vector_pairs.h"

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

test::define_test("test Empty impl", [] {
    using VP = VectorPairs<u32, u32, VectorPairsImpl::EMPTY>;
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

test::define_test("test Simple impl", [] {
    test_non_empty(VectorPairs<u32, u32, VectorPairsImpl::SIMPLE>());
});

test::define_test("test Dual impl", [] {
    test_non_empty(VectorPairs<u32, u32, VectorPairsImpl::DUAL>());
});

test::define_test("test Dual take", [] {
    auto vp = VectorPairs<u32, u32, VectorPairsImpl::DUAL>();
    vp.emplace_back(0, 5);
    vp.emplace_back(2, 6);
    assert(vp.size() == 2u);

    auto v1 = vp.take_v1();
    auto v2 = vp.take_v2();
    assert(vp.size() == 0u);
    assert(std::ranges::equal(v1, std::vector<u32> { 0, 2 }));
    assert(std::ranges::equal(v2, std::vector<u32> { 5, 6 }));
});

});
