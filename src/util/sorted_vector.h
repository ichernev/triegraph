#ifndef __SORTED_VECTOR_H__
#define __SORTED_VECTOR_H__

#include "util/util.h"
#include <vector>
#include <unordered_map>

namespace triegraph {

template <typename Beacon, typename Diff = u8>
struct SortedVector {

    SortedVector(u32 beacon_interval, u32 bits_per_diff = sizeof(Diff) * 8)
        : beacon_interval(beacon_interval),
          bits_per_diff(bits_per_diff),
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
        u64 sz = diffs.size();
        if (sz % beacon_interval == 0) {
            beacons.push_back(elem);
            diffs.push_back(0);
        } else if (elem - sum < diff_sentinel) {
            diffs.push_back(elem - sum);
        } else {
            diffs.push_back(diff_sentinel);
            of_diffs[sz] = elem - sum;
        }
        sum = elem;
    }

    Beacon operator[] (Beacon idx) const {
        auto dr = div(idx, beacon_interval);
        Beacon res = beacons[dr.quot];
        Beacon base_idx = idx - dr.rem;
        for (u32 i = 1; i <= dr.rem; ++i) {
            if (diffs[base_idx + i] != diff_sentinel)
                res += diffs[base_idx + i];
            else
                res += of_diffs.find(base_idx + i)->second;
        }
        return res;
    }

// private:
    u32 beacon_interval;
    u32 bits_per_diff;

    std::vector<Beacon> beacons;
    std::vector<Diff> diffs;
    std::unordered_map<Beacon, Beacon> of_diffs;

    Beacon sum; // used during build
    Diff diff_sentinel;
};

} /* namespace triegraph */


#endif /* __SORTED_VECTOR_H__ */
