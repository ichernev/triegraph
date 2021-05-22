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
        beacons.reserve((size - 1) / beacon_interval + 1);
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
        auto dr = div(idx, beacon_interval);
        Beacon res = beacons[dr.quot];
        Beacon base_idx = idx - dr.rem;
        for (u32 i = 1; i <= dr.rem; ++i) {
            res += get_diff(base_idx + i);
        }
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

    Beacon binary_search(Beacon elem) const {
        if (size() == 0)
            return 0;

        Beacon end = (size() - 1) / beacon_interval;
        if (beacons.at(end) <= elem) {
            return _linear_search(end * beacon_interval, size(),
                    elem - beacons[end]);
        }
        Beacon beg = 0;
        while (beg + 1 < end) {
            Beacon mid = beg + (end - beg) / 2;
            if (beacons.at(mid) <= elem)
                beg = mid;
            else
                end = mid;
        }
        return _linear_search(
                beg * beacon_interval,
                end * beacon_interval,
                elem - beacons.at(beg));
    }

    // TODO: Add random-access iterator, for bsrch and equal (tests)
private:
    u32 beacon_interval;
    u32 bits_per_diff;

    std::vector<Beacon> beacons;
    std::vector<Diff> diffs;
    std::unordered_map<Beacon, Beacon> of_diffs;

    // std::vector<Beacon> input;

    Beacon sum; // used during build
    Diff diff_sentinel;

    Beacon _linear_search(Beacon from_id, Beacon to_id, Beacon elem) const {
        Beacon total = 0;
        for (++from_id; from_id < to_id; ++from_id) {
            total += get_diff(from_id);
            if (total > elem)
                return --from_id;
        }
        return --to_id;
    }
};

} /* namespace triegraph */


#endif /* __SORTED_VECTOR_H__ */
