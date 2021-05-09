#ifndef __COMPACT_VECTOR_H__
#define __COMPACT_VECTOR_H__

#include "util/util.h"

#include <type_traits>
#include <vector>
#include <cstdlib> /* div */
#include <cassert>

namespace triegraph {

template <typename T>
struct CompactVector {
    static_assert(std::is_unsigned_v<T>);
    static constexpr u32 max_bits = sizeof(T) * BITS_PER_BYTE;
    static constexpr T lsh(T base, u32 shift) { return shift < max_bits ? (base << shift) : 0; }
    static constexpr T rsh(T base, u32 shift) { return shift < max_bits ? (base >> shift) : 0; }
    static constexpr T mask_(u32 bits) { return lsh(1, bits) - 1; }

    template <bool cnst>
    using data_iterator = std::conditional_t<cnst,
          typename std::vector<T>::const_iterator,
          typename std::vector<T>::iterator>;
    using Self = CompactVector;
    using value_type = T;

    CompactVector() : sz() {
        data.push_back(0);
        set_bits_(max_bits);
    }
    CompactVector(const CompactVector &) = delete;
    CompactVector(CompactVector &&) = default;
    CompactVector &operator= (const CompactVector &) = delete;
    CompactVector &operator= (CompactVector &&) = default;

    void set_bits_(u32 bits) {
        assert(0 < bits && bits <= max_bits);
        assert(size() == 0);
        this->mask = mask_(bits);
        this->bits = bits;
    }

    Self &set_bits(u32 bits) & { set_bits_(bits); return *this; }
    Self &&set_bits(u32 bits) && { set_bits_(bits); return std::move(*this); }

    void reserve(u64 size) {
        auto dr = div((size - 1) * bits, u64(max_bits));
        u64 ncap = dr.quot + 2;
        if (ncap > data.capacity()) {
            data.reserve(ncap);
        }
    }

    u64 capacity() const {
        if (data.capacity() < 2)
            return 0;
        // std::cerr << "computing capacity: " << (data.capacity() - 1) * max_bits / bits << std::endl;
        return div_up((data.capacity() - 1) * max_bits, bits);
    }

    u64 size() const {
        return sz;
    }

    void push_back(const T &val) {
        auto dr = div(sz * bits, u64(max_bits));
        if (dr.quot + 1 >= data.size())
            data.push_back(0);

        Ref<false>(data.begin(), dr, mask) = val;
        // data[dr.quot] |= (val & mask) << dr.rem;
        // data[dr.quot+1] |= (val & mask) >> (max_bits - dr.rem);

        ++ sz;
    }

    template <typename... Args>
    void emplace_back(Args&&... args) {
        return push_back(T(std::forward<Args>(args)...));
    }

    template <bool cnst>
    struct Ref {
        data_iterator<cnst> it;
        T mask;
        i32 rem;

        Ref(data_iterator<cnst> beg, const quot_rem<u64> &dr, T mask)
            : it(beg + dr.quot),
              mask(mask),
              rem(dr.rem)
        {}
        Ref(data_iterator<cnst> it, u32 rem, T mask)
            : it(it),
              mask(mask),
              rem(rem)
        {}
        Ref(const Ref &) = default;
        Ref(Ref &&) = default;

        Ref operator= (const T &val) {
            // std::cerr << std::endl
            //     << " it   " << std::hex << *it << std::dec
            //     << " it+1 " << std::hex << *(it+1) << std::dec
            //     << " pre assign val " << val
            //     << " rem " << rem << " mask " << mask
            //     << " sh1<< " << rem << " sh2>> " << (max_bits - rem)
            //     << " X= " << ((val & mask) >> (max_bits - rem))
            //     << std::endl;

            *it &= ~(mask << rem);
            *it |= (val & mask) << rem;
            *(it+1) &= ~rsh(mask, max_bits - rem);
            *(it+1) |= rsh(val & mask, max_bits - rem);

            // std::cerr
            //     << " it   " << std::hex << *it << std::dec
            //     << " it+1 " << std::hex << *(it+1) << std::dec
            //     << " post assign" << std::endl;

            return *this;
        }
        Ref operator= (const Ref &val) { return this->operator= (T(val)); }
        Ref operator= (Ref &&val) { return this->operator= (T(val)); }

        friend bool operator== (const Ref &a, const Ref &b) { return T(a) == T(b); }
        friend bool operator== (const Ref &a, const T &b) { return T(a) == b; }
        friend bool operator== (const T &a, const Ref &b) { return a == T(b); }
        friend std::strong_ordering operator<=> (const Ref &a, const Ref &b) {
            return T(a) <=> T(b);
        }
        friend std::strong_ordering operator<=> (const Ref &a, const T &b) {
            return T(a) <=> b;
        }
        friend std::strong_ordering operator<=> (const T &a, const Ref &b) {
            return a <=> T(b);
        }

        operator T() const {
            // std::cerr << "----" << std::hex << ((*it) & mask << rem) << " "
            //     << (*(it+1) & rsh(mask, max_bits - rem)) << std::dec << std::endl;
            return ((*it) >> rem & mask)
                | (lsh(*(it+1), max_bits - rem) & mask);
        }

        // void swap(const Ref b) const { T tmp = *this; *this = T(b); b = tmp; }
        // void swap(T &b) const { T tmp = *this; *this = b; b = tmp; }

        friend inline void swap(Ref a, Ref b) { T tmp = T(a); a = T(b); b = tmp; }
        friend inline void swap(Ref a, T &b) { T tmp = T(a); a = b; b = tmp; }
        friend inline void swap(T &a, Ref b) { T tmp = a; a = T(b); b = tmp; }
    };
    using reference_type = Ref<false>;

    T operator[] (u64 idx) const {
        return Ref<true>(data.begin(), div(idx * bits, u64(max_bits)), mask_(bits));
        // auto dr = div(idx * bits, max_bits);
        // return data[dr.quot] & mask << dr.rem |
        //     data[dr.quot+1] & mask >> (max_bits - dr.rem);
    }

    reference_type operator[] (u64 idx) {
        return { data.begin(), div(idx * bits, u64(max_bits)), mask_(bits) };
    }

    template <bool cnst>
    struct Iter {
        using iterator_category = std::random_access_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = T;
        using proxy_t = Ref<cnst>;
        using reference_type = std::conditional_t<cnst, T, proxy_t>;
        using Self = Iter;

        data_iterator<cnst> it;
        i32 rem;
        u32 bits;

        Iter() {}
        Iter(data_iterator<cnst> it, u32 rem, u32 bits)
            : it(it),
              rem(rem),
              bits(bits)
        {}
        Iter(const Iter &) = default;
        Iter(Iter &&) = default;
        Iter &operator= (const Iter &) = default;
        Iter &operator= (Iter &&) = default;

        void _inc() { rem += bits; if (u32(rem) >= max_bits) { ++ it; rem -= max_bits; } }
        void _dec() { rem -= bits; if (rem < 0) { -- it; rem += max_bits; } }
        void _add(difference_type d) {
            auto dr = std::div(rem + d * bits, difference_type(max_bits));
            if (dr.rem < 0) {
                dr.rem += max_bits;
                dr.quot -= 1;
            }
            it += dr.quot;
            rem = dr.rem;
        }

        reference_type operator* () const { return proxy_t(it, rem, mask_(bits)); }
        Self &operator++ () { _inc(); return *this; }
        Self &operator-- () { _dec(); return *this; }
        Self operator++ (int) { Self tmp = *this; ++(*this); return tmp; }
        Self operator-- (int) { Self tmp = *this; --(*this); return tmp; }
        bool operator== (const Self &other) const {
            return it == other.it && rem == other.rem;
        }
        std::strong_ordering operator<=> (const Self &other) const {
            if (auto res = it <=> other.it; res != 0)
                return res;
            return rem <=> other.rem;
        }
        Self &operator+= (difference_type i) { _add(i); return *this; }
        Self &operator-= (difference_type i) { _add(-i); return *this; }
        Self operator+ (difference_type i) const { Self tmp = *this; tmp += i; return tmp; }
        Self operator- (difference_type i) const { Self tmp = *this; tmp -= i; return tmp; }
        friend Self operator+ (difference_type i, const Self &it) { Self tmp = it; tmp += i; return tmp; }

        reference_type operator[] (difference_type i) const {
            return *(*this + i);
        }
        difference_type operator- (const Self &other) const {
            // std::cerr << "XDIF " << it - other.it << " " << difference_type(rem) - other.rem << std::endl;
            return ((it - other.it) * max_bits + (difference_type(rem) - other.rem)) / difference_type(bits);
        }
    };

    using iterator = Iter<false>;
    using const_iterator = Iter<true>;

    iterator begin() { return { data.begin(), 0, bits }; }
    iterator end() { return begin() + size(); }
    const_iterator begin() const { return { data.begin(), 0, bits }; }
    const_iterator end() const { return begin() + size(); }


    std::vector<T> data;
    T mask;
    u32 bits;
    u64 sz;
};

template <typename T>
void compact_vector_set_bits(CompactVector<T> &cv, u32 bits) { cv.set_bits(bits); }
template <typename T>
void compact_vector_set_bits(std::vector<T> &cv, u32 bits) { }

} /* namespace triegraph */

#endif /* __COMPACT_VECTOR_H__ */
