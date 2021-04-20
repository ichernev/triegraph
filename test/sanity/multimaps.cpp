#include "testlib/test.h"

// #include "dna_config.h"
// #include "manager.h"
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
// using TG = Manager<dna::DnaConfig<0>>;

using SMM = SimpleMultimap<u32, u32>;
using HMM = HybridMultimap<u32, u32, u32,
      std::unordered_map<u32, std::pair<u32, u32>>>;
template <typename SortedContainer>
using DMM = DenseMultimap<u32, u32, u32, SortedContainer>;
using SimpleSorted = std::vector<u32>;
using AdvSorted = SortedVector<u32>;

template <typename Multimap, typename RndGen = std::minstd_rand>
struct MultimapTester {
    static std::vector<std::pair<u32, u32>> gen_pairs(u32 size, u32 key_size, u32 val_size) {
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

    static void test(u32 size, u32 key_size, u32 val_size) {
        auto pairs = gen_pairs(size, key_size, val_size);
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

    void define_tests() {
        test::define_test("small-256", [] {
            test(256u, 128u, 128u);
        });
        test::define_test("medium-5k", [] {
            test(5000u, 3000u, 3000u);
        });
        test::define_test("medium-50k", [] {
            test(50000u, 30000u, 30000u);
        });
        test::define_test("medium-500k", [] {
            test(500000u, 300000u, 300000u);
        });
        test::define_test("big-50m", [] {
            test(50000000u, 30000000u, 30000000u);
        });
    }
};

int m = test::define_module(__FILE__, [] {
    test::register_test_class<MultimapTester<SMM>>("SMM");
    test::register_test_class<MultimapTester<HMM>>("HMM");
    test::register_test_class<MultimapTester<DMM<SimpleSorted>>>("DMM_simple");
    test::register_test_class<MultimapTester<DMM<AdvSorted>>>("DMM_advanced");
});
