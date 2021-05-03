#ifndef __SORTED_VECTOR_H__
#define __SORTED_VECTOR_H__

#include "util/util.h"
#include <vector>
#include <unordered_map>

namespace triegraph {

template <typename Beacon, typename Diff = u8>
struct SortedVector {
    using value_type = Beacon;

    SortedVector(u32 beacon_interval = 32, u32 bits_per_diff = sizeof(Diff) * 8)
        : beacon_interval(beacon_interval),
          bits_per_diff(bits_per_diff),
          sum(0),
          diff_sentinel(pow(2, bits_per_diff) - 1)
    {}

    SortedVector(const SortedVector &) = delete;
    SortedVector(SortedVector &&) = default;
    SortedVector &operator= (const SortedVector &) = delete;
    SortedVector &operator= (SortedVector &&) = default;

    void reserve(u64 size) {
        beacons.reserve(div_up(size, beacon_interval));
        diffs.reserve(size);
    }

    u64 size() const { return diffs.size(); }
    u64 capacity() const { return diffs.capacity(); }

    void push_back(Beacon elem) {
        // assert(elem >= sum);
        u64 sz = diffs.size();
        if (sz % beacon_interval == 0) {
            beacons.push_back(elem);
        }
        // push diffs even for beacons
        if (elem - sum < diff_sentinel) {
            diffs.push_back(elem - sum);
        } else {
            diffs.push_back(diff_sentinel);
            of_diffs[sz] = elem - sum;
        }
        // input.push_back(elem);
        sum = elem;
    }

    Beacon operator[] (Beacon idx) const {
        if (idx == _cache_idx) {
            return _cache_val;
        }
        if (idx == _cache_idx + 1) {
            ++ _cache_idx;
            _cache_val += get_diff(idx);
            return _cache_val;
        }
        if (idx == _cache_idx - 1) {
            _cache_val -= get_diff(_cache_idx --);
            return _cache_val;
        }
        auto dr = div(idx, beacon_interval);
        Beacon res = beacons[dr.quot];
        Beacon base_idx = idx - dr.rem;
        for (u32 i = 1; i <= dr.rem; ++i) {
            res += get_diff(base_idx + i);
        }
        _cache_idx = idx;
        _cache_val = res;
        return res;
    }

    Beacon get_diff(Beacon idx) const {
        return diffs[idx] != diff_sentinel ?
            diffs[idx] : of_diffs.find(idx)->second;
    }

    Beacon at(Beacon idx) const {
        if (idx >= size()) {
            throw "idx-out-of-range";
        }
        return operator[](idx);
    }

    // void sanity_check() {
    //     assert(input.size() == size());
    //     for (u64 i = 0; i < size(); ++i) {
    //         if (input[i] != operator[](i)) {
    //             std::cerr << "i " << i << std::endl;
    //             for (u64 j = i < 40 ? 0 : i - 40; j < i+40; ++j) {
    //                 std::cerr << operator[](j) << "-" << input[j]<< " ";
    //             }
    //             std::cerr << std::endl;
    //         }
    //         assert(input[i] == operator[](i));
    //     }
    // }

    static SortedVector from_elem_seq(std::ranges::sized_range auto &&range) {
        SortedVector res;
        auto beg = range.begin();
        auto end = range.end();

        value_type max = *(--end) + 1; ++end;
        // std::cerr << "max is " << max << std::endl;
        res.reserve(max);

        value_type pos = 0;
        res.push_back(pos);
        -- max;
        for (value_type id = 0; id < max; ++id) {
            while (beg != end && *beg == id) {
                ++ beg;
                ++ pos;
            }
            res.push_back(pos);
        }
        return res;
    }

private:
    u32 beacon_interval;
    u32 bits_per_diff;

    std::vector<Beacon> beacons;
    std::vector<Diff> diffs;
    std::unordered_map<Beacon, Beacon> of_diffs;

    mutable Beacon _cache_val;
    mutable Beacon _cache_idx;
    // std::vector<Beacon> input;

    Beacon sum; // used during build
    Diff diff_sentinel;
};

} /* namespace triegraph */


#endif /* __SORTED_VECTOR_H__ */
