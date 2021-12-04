// SPDX-License-Identifier: MPL-2.0
/*
 * Copyright (c) 2021, Iskren Chernev
 */

#include "testlib/test.h"

#include "util/dense_multimap.h"
#include "util/hybrid_multimap.h"
#include "util/simple_multimap.h"
#include "util/sorted_vector.h"
#include "util/util.h"

#include <algorithm>
#include <unordered_map>
#include <utility>
#include <vector>
#include <random>

using namespace triegraph;

using SMM = SimpleMultimap<u32, u32>;
using HMM = HybridMultimap<u32, u32, u32,
      std::unordered_map<u32, std::pair<u32, u32>>>;
template <typename SortedContainer>
using DMM = DenseMultimap<u32, u32, SortedContainer>;
using SimpleSorted = std::vector<u32>;
using AdvSorted = SortedVector<u32>;

template <typename Multimap, typename RndGen = std::minstd_rand>
struct MultimapTester : public test::TestCaseBase {
    u32 size, key_size, val_size;
    std::vector<std::pair<u32, u32>> pairs;

    MultimapTester(std::string &&name, u32 size, u32 key_size, u32 val_size)
        : TestCaseBase(std::move(name)),
          size(size),
          key_size(key_size),
          val_size(val_size)
    {}

    static std::vector<std::pair<u32, u32>> gen_pairs(
            u32 size, u32 key_size, u32 val_size) {
        auto gen = RndGen();
        std::vector<std::pair<u32, u32>> pairs;
        pairs.reserve(size);

        for (u32 i = 0; i < size; ++i) {
            u32 key = gen() % key_size;
            u32 val = gen() % val_size;
            pairs.emplace_back(key, val);
        }

        std::ranges::sort(pairs);
        auto ur = std::ranges::unique(pairs);
        pairs.resize(ur.begin() - pairs.begin());

        return pairs;
    }

    virtual void prepare() {
        pairs = gen_pairs(size, key_size, val_size);
    }

    void run() {
        auto mm = Multimap(pairs);

        assert(test::equal_sorted(mm, pairs));
        for (auto const &p : pairs) {
            auto lb = std::lower_bound(pairs.begin(), pairs.end(),
                    std::make_pair(p.first, 0u));
            auto ub = std::upper_bound(pairs.begin(), pairs.end(),
                    std::make_pair(p.first, std::numeric_limits<u32>::max()));
            assert(test::equal_sorted(
                        mm.values_for(p.first),
                        std::ranges::subrange(lb, ub)
                            | std::ranges::views::transform([] (const auto &p) {
                                return p.second; })));
        }
        auto keys = std::vector<u32> {};
        std::ranges::copy(
                pairs | std::ranges::views::transform(&std::pair<u32, u32>::first),
                std::back_inserter(keys));
        std::ranges::sort(keys);
        auto uk = std::ranges::unique(keys);
        keys.resize(uk.begin() - keys.begin());

        assert(test::equal_sorted(mm.keys(), keys));
   }

    using Self = MultimapTester;

    static void define_tests(std::string pref) {
        pref += "::";
        test::add_test<Self>(pref + "small-256", 256u, 128u, 128u);
        test::add_test<Self>(pref + "medium-5k", 5000u, 3000u, 3000u);
        test::add_test<Self>(pref + "medium-50k", 50000u, 30000u, 30000u);
        test::add_test<Self>(pref + "medium-500k", 500000u, 300000u, 300000u);
        test::add_test<Self>(pref + "big-50m", 50000000u, 30000000u, 30000000u);
    }
};

int m = test::define_module(__FILE__, [] {
    MultimapTester<SMM>::define_tests("SMM");
    MultimapTester<HMM>::define_tests("HMM");
    MultimapTester<DMM<SimpleSorted>>::define_tests("DMM_simple");
    MultimapTester<DMM<AdvSorted>>::define_tests("DMM_adv");
});
