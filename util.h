#ifndef __UTIL_H__
#define __UTIL_H__

namespace triegraph {

using u8 = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using u64 = unsigned long long;
constexpr int BITS_PER_BYTE = 8;

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

} /* namespace triegraph */

#endif /* __UTIL_H__ */
