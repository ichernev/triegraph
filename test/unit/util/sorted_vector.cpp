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

        assert(sv.size() == elems.size());
        for (u32 i = 0; i < sv.size(); ++i) {
            // std::cerr << i << " " << sv[i] << std::endl;
            assert(sv[i] == elems[i]);
        }
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

        // for (u32 i = 0; i < sv.size(); ++i) {
        //     std::cerr << i << " " << sv[i] << std::endl;
        // }
        assert(sv.size() == elems.size());
        for (u32 i = 0; i < sv.size(); ++i) {
            assert(sv[i] == elems[i]);
        }
    });
});
