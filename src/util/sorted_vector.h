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

    // quicker than get_diff(idx) == 0
    bool is_zero_diff(Beacon idx) const { return diffs[idx] == 0; }

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

    // static SortedVector from_elem_seq(std::ranges::sized_range auto &&range) {
    //     SortedVector res;
    //     auto beg = range.begin();
    //     auto end = range.end();

    //     value_type max = *(--end); ++end;
    //     // std::cerr << "max is " << max << std::endl;
    //     res.reserve(max + 1);

    //     value_type pos = 0;
    //     res.push_back(pos);
    //     for (value_type id = 0; id < max; ++id) {
    //         while (beg != end && *beg == id) {
    //             ++ beg;
    //             ++ pos;
    //         }
    //         res.push_back(pos);
    //     }
    //     return res;
    // }

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

    struct Iter {
        using Parent = SortedVector;
        using Self = Iter;

        using iterator_category = std::random_access_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = Beacon;
        using reference = value_type;

        const Parent *parent;
        Beacon id;
        Beacon val;

        Iter() {}
        Iter(const Parent &parent, Beacon id)
            : parent(&parent),
              id(id),
              val(_xid(id)) {}

        Iter(const Iter &) = default;
        Iter(Iter &&) = default;
        Iter &operator= (const Iter &) = default;
        Iter &operator= (Iter &&) = default;

        // All this crap is because iterators can point to 1-after-end,
        // so this iterator should seamlessly transition back onto the
        // sequence.
        Beacon _xget_diff(Beacon idx) const {
            if (idx == this->parent->size())
                return 0;
            return this->parent->get_diff(idx);
        }
        Beacon _xid(Beacon id) const {
            auto sz = this->parent->size();
            if (id < sz)
                return this->parent->operator[](id);
            else if (sz)
                return this->parent->operator[](sz-1);
            else
                return 0;
        }
        void _inc() { val += _xget_diff(++id); }
        void _dec() { val -= _xget_diff(id--); }
        void _add(difference_type d) {
            if (0 < d && d <= 4)
                while (d--) _inc();
            else if (-4 <= d && d < 0)
                while (d++) _dec();
            else
                val = _xid(id += d);
        }

        reference operator* () const { return val; }
        Self &operator++ () { _inc(); return *this; }
        Self &operator-- () { _dec(); return *this; }
        Self operator++ (int) { Self tmp = *this; ++(*this); return tmp; }
        Self operator-- (int) { Self tmp = *this; --(*this); return tmp; }
        bool operator== (const Self &other) const { return id == other.id; }
        std::strong_ordering operator<=> (const Self &other) const { return id <=> other.id; }
        Self &operator+= (difference_type i) { _add(+i); return *this; }
        Self &operator-= (difference_type i) { _add(-i); return *this; }
        Self operator+ (difference_type i) const { Self tmp = *this; tmp += i; return tmp; }
        Self operator- (difference_type i) const { Self tmp = *this; tmp -= i; return tmp; }
        friend Self operator+ (difference_type i, const Self &it) { Self tmp = it; tmp += i; return tmp; }

        reference operator[] (difference_type i) const { return *(*this + i); }
        difference_type operator- (const Self &other) const {
            return difference_type(id) - other.id; }

        void skip_empty() {
            auto sz = this->parent->size();
            if (sz == 0) return;
            --sz;
            while (id < sz) {
                if (this->parent->is_zero_diff(id+1))
                    ++id;
                else
                    break;
            }
            // id == size-1 is never empty, by construction.
        }
    };

    using const_iterator = Iter;
    const_iterator begin() const { return Iter(*this, 0); }
    const_iterator end() const { return Iter(*this, size()); }

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

template <typename SortedVector>
SortedVector sorted_vector_from_elem_seq(std::ranges::sized_range auto &&range) {
    using value_type = SortedVector::value_type;
    SortedVector res;
    auto beg = range.begin();
    auto end = range.end();

    value_type max = *(--end); ++end;
    // std::cerr << "max is " << max << std::endl;
    res.reserve(max + 1);

    value_type pos = 0;
    res.push_back(pos);
    for (value_type id = 0; id < max; ++id) {
        while (beg != end && *beg == id) {
            ++ beg;
            ++ pos;
        }
        res.push_back(pos);
    }
    return res;
}

template <typename T>
inline constexpr bool is_sorted_vector_v = false;

template <typename Beacon, typename Diff>
inline constexpr bool is_sorted_vector_v<SortedVector<Beacon, Diff>> = true;

} /* namespace triegraph */


#endif /* __SORTED_VECTOR_H__ */
