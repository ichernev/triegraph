#ifndef __UTIL_HUMAN_H__
#define __UTIL_HUMAN_H__

#include <sstream>
#include <concepts>
#include <type_traits>

namespace triegraph {

std::string to_human(
        std::integral auto val,
        bool add_sign,
        std::initializer_list<const char *> suff) {
    decltype(val) amt = val;
    if constexpr (std::is_signed_v<decltype(val)>) {
        amt = std::abs(val);
    }
    auto beg = suff.begin();
    auto end = suff.end();
    -- end; // if we reach the end we're already too late

    decltype(val) rem = 0;
    while (beg != end && amt >= 1000) {
        rem = amt % 1000;
        amt /= 1000;
        ++ beg;
    }

    std::ostringstream res;
    if (std::is_signed_v<decltype(val)> && add_sign) {
        res << (val >= 0 ? '+' : '-');
    }
    res << amt;
    if (beg != suff.begin()) {
        if (amt < 10) {
            res << "." << (rem / 100) << (rem / 10) % 10;
        } else if (amt < 100) {
            res << "." << (rem / 100);
        }
    }
    res << *beg;
    return res.str();
}

std::string to_human_mem(std::integral auto mem_kb, bool add_sign = true) {
    return to_human(mem_kb, add_sign, {"kb", "mb", "gb"});
}

std::string to_human_time(std::integral auto time_ms, bool add_sign = true) {
    return to_human(time_ms, add_sign, {"ms", "s"});
}

std::string to_human_number(std::integral auto num, bool add_sign = true) {
    return to_human(num, add_sign, {"", "k", "m", "b"});
}

} /* namespace triegraph */


#endif /* __UTIL_HUMAN_H__ */
