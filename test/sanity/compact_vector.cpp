#include "util/util.h"
#include "util/compact_vector.h"

#include "testlib/test.h"

#include <vector>
#include <numeric>
#include <algorithm>
#include <random>

using namespace triegraph;

using CV32 = triegraph::CompactVector<u32>;
using CV64 = triegraph::CompactVector<u64>;

template <typename CV, u32 bits>
struct CompactVectorTester : public test::TestCaseBase {
    CompactVectorTester(const std::string &name, u64 size)
        : TestCaseBase(name), size(size)
    {}

    void run() {
        auto v = std::vector<typename CV::value_type>(size);
        std::iota(v.begin(), v.end(), 0);

        auto cv = CV().set_bits(bits);
        cv.reserve(size);
        std::ranges::copy(std::ranges::iota_view {u64(0), size}, std::back_inserter(cv));
        assert(std::ranges::equal(v, cv));

        std::shuffle(v.begin(), v.end(), std::mt19937 {});
        std::shuffle(cv.begin(), cv.end(), std::mt19937 {});
        assert(std::ranges::equal(v, cv));
        // for (u32 i = 0; i < vec.size(); ++i) {
        //     assert(vec[i] == cvec[i]);
        // }
        std::sort(v.begin(), v.end());
        std::sort(cv.begin(), cv.end());
        assert(std::ranges::equal(v, cv));
    }

    u64 size;
};

int m = test::define_module(__FILE__, [] {
    test::add_test<CompactVectorTester<CV32, 16>>("sanity::100", 100);
    test::add_test<CompactVectorTester<CV32, 31>>("sanity::1_000_000", 1000000);
    test::add_test<CompactVectorTester<CV64, 33>>("sanity::100_000_000", 100000000);
});
