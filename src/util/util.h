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

using i8 = signed char;
using i16 = signed short;
using i32 = signed int;
using i64 = signed long long;

constexpr u64 pow(u64 base, u64 power) {
    u64 res = 1;
    while (power --) {
        res *= base;
    }
    return res;
}

constexpr u32 log2_ceil(u64 value) {
    u32 res = 0;
    while (value > 1) {
        res += 1;
        value /= 2;
    }
    return res;
}

constexpr u32 log4_ceil(u64 value) {
    int res = 0;
    while (value > 1) {
        res += 1;
        value /= 4;
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

template <bool, typename A, typename B>
struct choose_type { using type = A; };

template<typename A, typename B>
struct choose_type<false, A, B> { using type = B; };

template <bool b, typename A, typename B>
using choose_type_t = choose_type<b, A, B>::type;

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
    iter_codec(const Self &) = default;
    iter_codec(Self &&) = default;
    Self &operator= (const Self &) = default;
    Self &operator= (Self &&) = default;

    reference_type operator* () const { return Codec::to_ext(*it); }
    Self &operator++ () { ++it; return *this; }
    Self operator++ (int) { Self tmp = *this; ++(*this); return tmp; }
    template <typename OtherIt, typename OtherCodec>
    bool operator== (const iter_codec<OtherIt, OtherCodec> &other) const { return it == other.it; }

    [[no_unique_address]] IT it; // this could be sentinel

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

    iter_pair(IT1 it1 = {}, IT2 it2 = {}): first(it1), second(it2) {}
    template<std::ranges::range R>
    iter_pair(const R &other)
        : first(other.begin()), second(other.end()) {}
    template<std::ranges::range R>
    Self &operator= (const R &other) {
        first = other.begin(); second = other.end(); return *this;
    }

    iter_pair(const iter_pair &) = default;
    iter_pair(iter_pair &&) = default;
    Self &operator= (const iter_pair &) = default;
    Self &operator= (iter_pair &&) = default;

    iter_codec<IT1, Codec> first;
    [[no_unique_address]] iter_codec<IT2, Codec> second;

    decltype(first) begin() const noexcept { return first; }
    decltype(second) end() const noexcept { return second; }

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
