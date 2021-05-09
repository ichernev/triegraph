#include "testlib/test.h"

#include "util/util.h"
#include "util/compact_vector.h"
#include "util/vector_pairs.h"

#include <utility>
#include <iterator>
#include <algorithm>

using namespace triegraph;
using CV32 = CompactVector<u32>;
using CV64 = CompactVector<u64>;

template <typename CV, u32 bits>
struct CompactVectorTester {
    static CV make_cv() {
        return CV().set_bits(bits);
    }

    static void test_empty() {
        auto cv = make_cv();

        assert(cv.size() == 0);
        assert(cv.capacity() == 0);

        assert(cv.begin() == cv.end());
        assert(std::as_const(cv).begin() == std::as_const(cv).end());
    }

    static void test_tiny_(CV cv) {
        cv.push_back(5);

        assert(cv.size() == 1);
        assert(cv.capacity() >= 1);

        // std::cerr << "cv[0] " << typename CV::value_type(cv[0]) << std::endl;
        assert(cv[0] == 5);
        assert(std::as_const(cv)[0] == 5);

        assert(*cv.begin() == 5);
        assert(*std::as_const(cv).begin() == 5);
        assert(cv.end() - cv.begin() == 1);
        assert(++cv.begin() == cv.end());
        assert(std::as_const(cv).end() - std::as_const(cv).begin() == 1);
        assert(++std::as_const(cv).begin() == std::as_const(cv).end());
    }

    static void test_tiny() {
        test_tiny_(make_cv());
    }

    static void test_tiny2() {
        auto cv = make_cv();
        cv.reserve(1);
        test_tiny_(std::move(cv));
    }

    static void test_small() {
        auto cv = make_cv();

        cv.push_back(2);
        cv.push_back(4);

        assert(cv.size() == 2);
        // std::cerr << "cap = " << cv.capacity() << std::endl;
        assert(cv.capacity() >= 2);

        // std::cerr << std::hex << cv.data[0] << std::dec << std::endl;
        // std::cerr << cv[0] << " " << cv[1] << std::endl;
        assert(cv[0] == 2 && cv[1] == 4);

        swap(cv[0], cv[1]);
        assert(cv[0] == 4 && cv[1] == 2);

        cv[0] = 5;
        assert(cv[0] == 5 && cv[1] == 2);
        cv[1] = cv[0];
        assert(cv[0] == 5 && cv[1] == 5);

        // std::cerr << "WOOO" << std::endl;
    }

    static void test_concepts() {
        auto func_it = [](std::random_access_iterator auto &&) {};
        auto func_rng = [](std::ranges::random_access_range auto &&) {};
        // auto func_iw = [](std::indirectly_writable<u32> auto &&) {};
        auto cv = CV32();
        func_it(cv.begin());
        func_it(std::as_const(cv).begin());

        func_rng(cv);
        func_rng(std::as_const(cv));

        // func_iw(cv.begin());
        // func_iw(std::vector<int>().begin());
        // std::ranges::sort(std::vector<bool>());
        // std::ranges::sort(VectorPairs<int, int, VectorPairsImpl::DUAL> {});
    }

    static void test_small_sort() {
        auto cv = make_cv();

        cv.push_back(3);
        cv.push_back(0);
        cv.push_back(2);
        cv.push_back(1);
        // std::ranges::sort doesn't work with proxy iterators
        std::sort(cv.begin(), cv.end());
        // std::ranges::sort(cv, std::less<typename CV::value_type> {});

        // for (u32 i = 0; i < 4; ++i) {
        //     std::cerr << cv[i] << std::endl;
        // }

        auto beg = cv.begin();
        auto cbeg = std::as_const(cv).begin();
        for (u32 i = 0; i < 4; ++i) {
            assert(cv[i] == i);
            assert(std::as_const(cv)[i] == i);
            assert(*beg == i);
            assert(*cbeg == i);
            ++ beg;
            ++ cbeg;
        }
        assert(beg == cv.end());
        assert(cbeg == std::as_const(cv).end());
    }

    static void define_tests() {
        using Self = CompactVectorTester;

        std::ostringstream os;
        os << sizeof(typename CV::value_type) << "::" << bits << "::";
        std::string pref = os.str();

        test::define_test(pref + "empty", &Self::test_empty);
        test::define_test(pref + "tiny", &Self::test_tiny);
        test::define_test(pref + "tiny2", &Self::test_tiny2);
        test::define_test(pref + "small", &Self::test_small);
        test::define_test(pref + "concepts", &Self::test_concepts);
        test::define_test(pref + "small_sort", &Self::test_small_sort);
    }
};

int m = test::define_module(__FILE__, [] {
    CompactVectorTester<CV32, 32>::define_tests();
    CompactVectorTester<CV32, 31>::define_tests();
    CompactVectorTester<CV32, 17>::define_tests();
    CompactVectorTester<CV32, 15>::define_tests();
    CompactVectorTester<CV32, 6>::define_tests();

    CompactVectorTester<CV64, 64>::define_tests();
    CompactVectorTester<CV64, 63>::define_tests();
    CompactVectorTester<CV64, 35>::define_tests();
    CompactVectorTester<CV64, 33>::define_tests();
    CompactVectorTester<CV64, 31>::define_tests();
    CompactVectorTester<CV64, 17>::define_tests();
    CompactVectorTester<CV64, 15>::define_tests();
    CompactVectorTester<CV64, 6>::define_tests();
});

// test::define_test("empty", [] {
//     CompactVectorTester<CV32>(CV32()).test_empty();
//     CompactVectorTester<CV32>(CV32().set_bits(15)).test_empty();
//     CompactVectorTester<CV32>(CV32().set_bits(6)).test_empty();
// });

// test::define_test("tiny", [] {
//     CompactVectorTester<CV32>(CV32()).test_tiny();
//     CompactVectorTester<CV32>(CV32().set_bits(15)).test_tiny();
//     CompactVectorTester<CV32>(CV32().set_bits(6)).test_tiny();
// });

// test::define_test("tiny2", [] {
//     CompactVectorTester<CV32>(CV32()).test_tiny2();
//     CompactVectorTester<CV32>(CV32().set_bits(15)).test_tiny2();
//     CompactVectorTester<CV32>(CV32().set_bits(6)).test_tiny2();
// });

// test::define_test("iter concep", [] {
// });

// test::define_test("small", [] {
//     CompactVectorTester<CV32>(CV32()).test_small();
//     CompactVectorTester<CV32>(CV32().set_bits(15)).test_small();
//     CompactVectorTester<CV32>(CV32().set_bits(6)).test_small();
// });


// test::define_test("small sort", [] {
//     CompactVectorTester<CV32>(CV32()).test_small_sort();
//     CompactVectorTester<CV32>(CV32().set_bits(15)).test_small_sort();
//     CompactVectorTester<CV32>(CV32().set_bits(6)).test_small_sort();
// });
