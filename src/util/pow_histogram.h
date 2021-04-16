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
    PowHistogram(std::ranges::input_range auto&& range) {
        add(std::forward<decltype(range)>(range));
    }

    void add(std::ranges::input_range auto&& range) {
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
            cumul.resize(b+1, 0);
        }
        bins[b] += 1;
        cumul[b] += n;
    }

    void print(std::ostream &os) const {
        auto rit = std::ranges::find_if(bins.rbegin(), bins.rend(),
                [](auto b) { return b != 0; });
        auto it = bins.end() - (rit - bins.rbegin());

        u64 btotal = std::accumulate(bins.begin(), it, 0llu);
        u64 ctotal = std::accumulate(cumul.begin(), cumul.end(), 0llu);
        int bi = 0;
        u64 b_psum = 0;
        u64 c_psum = 0;
        for (const auto &b : std::ranges::subrange(bins.begin(), it)) {
            b_psum += b;
            c_psum += cumul[bi];
            os << bi << " - "
                << b << " (" << double(b_psum)/btotal << ") "
                << cumul[bi] << " [" << double(c_psum)/ctotal << "]"
                << std::endl;
            ++bi;
        }
    }

private:
    std::vector<u64> bins;
    std::vector<u64> cumul;
};

} /* namespace triegraph */

#endif /* __POW_HISTOGRAM_H__ */
