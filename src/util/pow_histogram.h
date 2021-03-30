#ifndef __POW_HISTOGRAM_H__
#define __POW_HISTOGRAM_H__

#include "util/util.h"

#include <concepts>
#include <numeric>
#include <type_traits>
#include <vector>
#include <algorithm>

namespace triegraph {

struct PowHistogram {

    PowHistogram() {}
    PowHistogram(std::ranges::range auto const &range) {
        add(range);
    }

    void add(std::ranges::range auto const &range) {
        for (const auto &n : range) {
            add(n);
        }
    }

    void add(std::signed_integral auto const& n) {
        if (n >= 0) {
            add(std::make_unsigned_t<std::decay_t<decltype(n)>>(n));
        } else {
            throw "add negative number to histogram";
        }
    }

    void add(std::unsigned_integral auto const& n) {
        auto b = log2_ceil(n);
        if (b >= bins.size()) {
            bins.resize(b+1, 0);
        }
        ++ bins[b];
    }

    void print(std::ostream &os) const {
        auto rit = std::ranges::find_if(bins.rbegin(), bins.rend(),
                [](auto b) { return b != 0; });
        auto it = bins.end() - (rit - bins.rbegin());

        u64 total = std::accumulate(bins.begin(), it, 0llu);
        int bi = 0;
        u64 ctotal = 0;
        for (const auto &b : std::ranges::subrange(bins.begin(), it)) {
            ctotal += b;
            os << bi++ << " - " << b << " (" << double(ctotal)/total << ")" << std::endl;
        }
    }

private:
    std::vector<u64> bins;
};

} /* namespace triegraph */

#endif /* __POW_HISTOGRAM_H__ */
