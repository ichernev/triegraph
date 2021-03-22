#ifndef __UTIL_H__
#define __UTIL_H__

#include <iterator>
#include <ranges>
#include <iostream>

namespace triegraph {

using u8 = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using u64 = unsigned long long;
constexpr int BITS_PER_BYTE = 8;

constexpr u64 pow(u64 base, u64 power) {
    u64 res = 1;
    while (power --) {
        res *= base;
    }
    return res;
}

constexpr int log2_ceil(u64 value) {
    int res = 0;
    while (value > 1) {
        res += 1;
        value /= 2;
    }
    return res;
}

constexpr u64 div_up(u64 a, u64 b) {
    return (a + b - 1) / b;
}

template <typename T>
struct quot_rem {
    T quot;
    T rem;
};

template <typename T>
quot_rem<u32> div(T a, T b) {
    T quot = a / b;
    return { quot, a - b * quot };
}

template <typename Ext, typename Int = Ext>
struct CodecIdentity {
    using ext_type = Ext;
    using int_type = Int;

    static int_type to_int(const ext_type &ext) { return ext; }
    static ext_type to_ext(const int_type &in) { return in; }
};

template <typename IT,
         typename Codec = CodecIdentity<typename std::iterator_traits<IT>::value_type>>
struct iter_codec {
    using iterator_category = std::input_iterator_tag; // downgrade to input for now
    using difference_type = std::ptrdiff_t;
    using value_type = Codec::ext_type;
    using reference_type = value_type;
    using Self = iter_codec;
    // using SelfSimilar = iter_codec<IT,
    //       CodecIdentity<typename std::iterator_traits<IT>::value_type>;

    iter_codec(IT it = {}) : it(it) {}
    // iter_codec(SelfSimilar it) : it(it) {}


    iter_codec(const iter_codec &) = default;
    iter_codec(iter_codec &&) = default;
    Self &operator= (const iter_codec &) = default;
    Self &operator= (iter_codec &&) = default;

    reference_type operator* () const { return Codec::to_ext(*it); }
    Self &operator++ () { ++it; return *this; }
    Self operator++ (int) { Self tmp = *this; ++(*this); return tmp; }
    bool operator== (const Self &other) const { return it == other.it; }

    IT it;

    template <typename OtherCodec>
    operator iter_codec<IT, OtherCodec>() const { return {it}; }
};

template <typename IT1, typename IT2 = IT1,
         typename Codec = CodecIdentity<typename std::iterator_traits<IT1>::value_type>>
struct iter_pair : std::ranges::view_base {
    static_assert(std::is_same_v<
            typename Codec::int_type,
            typename std::iterator_traits<IT1>::value_type>);
    using value_type = Codec::ext_type;
    using Self = iter_pair;

    iter_pair() : first(), second() {}
    template<std::ranges::range R>
    iter_pair(const R &other)
        : first(other.begin()), second(other.end()) {}
    template<std::ranges::range R>
    Self &operator= (const R &other) {
        first = other.begin(); second = other.end(); return *this;
    }

    iter_pair(iter_pair &&) = default;
    Self &operator= (iter_pair &&) = default;

    iter_codec<IT1, Codec> first;
    [[no_unique_address]] iter_codec<IT2, Codec> second;
    iter_pair(IT1 begin, IT2 end) : first(begin), second(end) {}

    decltype(first) begin() const { return first; }
    decltype(second) end() const { return second; }

    value_type operator* () const { return *first; }
    Self &operator++ () { ++first; return *this; }
    Self &operator++ (int) { Self tmp = *this; ++(*this); return tmp; }
    bool empty() const { return first == second; }

    operator std::pair<iter_codec<IT1, Codec>, iter_codec<IT2, Codec>>() const { return { first, second }; }
};

} /* namespace triegraph */

template <typename IT1, typename IT2>
inline constexpr bool std::ranges::enable_borrowed_range<triegraph::iter_pair<IT1, IT2>> = true;

template <typename A, typename B>
std::ostream &operator<< (std::ostream &os, const std::pair<A, B> &p) {
    return os << p.first << ": " << p.second;
}

#endif /* __UTIL_H__ */