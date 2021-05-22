#include "util/sorted_vector.h"
#include "util/util.h"

#include "testlib/test.h"

#include <vector>
#include <algorithm>

using namespace triegraph;

template <typename B, typename D>
using SV = triegraph::SortedVector<B, D>;

int m = test::define_module(__FILE__, [] {
    test::define_test("small no of", [] {
        auto sv = SV<u32, u8>(4);
        auto elems = std::vector<u32> { 3, 3, 4, 9, 9, 9, 15, 20 };

        sv.reserve(elems.size());
        assert(elems.size() == sv.capacity());

        u32 sz = 0;
        for (const auto &el : elems) {
            sv.push_back(el);
            assert(++sz == sv.size());
            // std::cerr << "Pushing " << el << std::endl;
            // std::ranges::copy(sv.beacons, std::ostream_iterator<u32>(std::cerr, " ")); std::cerr << std::endl;
            // std::ranges::copy(sv.diffs, std::ostream_iterator<u32>(std::cerr, " ")); std::cerr << std::endl;
        }

        assert(std::ranges::equal(sv, elems));
    });

    test::define_test("small with overflow", [] {
        auto sv = SV<u32, u8>(4);
        auto elems = std::vector<u32> { 3, 5000, 5001, 5002, 5003, 25000, 25001, 25002 };

        sv.reserve(elems.size());
        assert(elems.size() == sv.capacity());

        u32 sz = 0;
        for (const auto &el : elems) {
            sv.push_back(el);
            assert(++sz == sv.size());
            // std::cerr << "Pushing " << el << std::endl;
            // std::ranges::copy(sv.beacons, std::ostream_iterator<u32>(std::cerr, " ")); std::cerr << std::endl;
            // std::ranges::copy(sv.diffs, std::ostream_iterator<u32>(std::cerr, " ")); std::cerr << std::endl;
        }

        assert(std::ranges::equal(sv, elems));
    });

    test::define_test("iter", [] {
        auto func_rng = [](std::ranges::random_access_range auto&&) {};
        func_rng(SV<u32, u8> {});

        auto func_it = [](std::random_access_iterator auto &&) {};
        func_it(SV<u32, u8>().begin());

        auto sv = SV<u32, u8> {};
        sv.reserve(6);
        std::ranges::copy(std::vector<u32> {0, 1, 2, 3, 4, 5}, std::back_inserter(sv));

        assert(*sv.begin() == 0);
        assert(sv.begin()[0] == 0);
        assert(sv.begin()[1] == 1);
        assert(sv.begin()[2] == 2);
        assert(sv.begin()[3] == 3);
        assert(sv.begin()[4] == 4);
        assert(sv.begin()[5] == 5);

        assert(*(sv.end() - 1) == 5);
        assert(sv.end()[-1] == 5);
        assert(sv.end()[-2] == 4);
        assert(sv.end()[-3] == 3);
        assert(sv.end()[-4] == 2);
        assert(sv.end()[-5] == 1);
        assert(sv.end()[-6] == 0);

        assert(sv.end() - sv.begin() == 6);
        assert(sv.begin() + 6 == sv.end());
    });

    test::define_test("from_elem_seq", [] {
        auto inp = std::vector<u32> { 0, 0, 0, 1, 1, 5, 7 };
        auto sv = sorted_vector_from_elem_seq<SV<u32, u8>>(inp);
        // std::cerr << sv.size() << std::endl;
        assert(sv.size() == 8); // 7 + 1
        auto exp = std::vector<u32> {0, 3, 5, 5, 5, 5, 6, 6};
        for (u32 i = 0; i < sv.size(); ++i) {
            // std::cerr << i << " " << sv[i] << std::endl;
            assert(sv[i] == exp[i]);
        }
    });

    test::define_test("vector from_elem_seq", [] {
        auto inp = std::vector<u32> { 0, 0, 0, 1, 1, 5, 7 };
        auto sv = sorted_vector_from_elem_seq<std::vector<u32>>(inp);
        // std::cerr << sv.size() << std::endl;
        assert(sv.size() == 8); // 7 + 1
        auto exp = std::vector<u32> {0, 3, 5, 5, 5, 5, 6, 6};
        for (u32 i = 0; i < sv.size(); ++i) {
            // std::cerr << i << " " << sv[i] << std::endl;
            assert(sv[i] == exp[i]);
        }
    });
});
