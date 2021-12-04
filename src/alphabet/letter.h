// SPDX-License-Identifier: MPL-2.0
/*
 * Copyright (c) 2021, Iskren Chernev
 */

#ifndef __LETTER_H__
#define __LETTER_H__

#include "util/util.h"

#include <type_traits>
#include <ostream>

namespace triegraph {

template <typename Holder_, u64 options_, typename Codec_ = CodecIdentity<Holder_>>
struct Letter {
    using Holder = Holder_;
    using Codec = Codec_;
    using Human = Codec::ext_type;
    static constexpr u64 num_options = options_;
    static constexpr Holder EPS = num_options;
    static constexpr int bits = log2_ceil(num_options);
    static constexpr Holder mask = (1 << bits) - 1;

    template <typename T>
        requires (std::is_integral<T>::value && !std::is_signed<T>::value &&
                sizeof(T) > sizeof(Holder))
    constexpr Letter(T h) : data(Holder(h)) { }
    constexpr Letter(Holder h = 0) : data(h) { }
    // Letter(&&)
    // operator=(&)
    // operator=(&&)

    constexpr operator Holder() const { return data; }
    bool operator== (const Letter &other) const  = default;
    auto operator<=> (const Letter &other) const = default;
    template <typename T>
        requires (std::is_integral<T>::value)
    bool operator== (T other) { return data == other; }

    Letter rev_comp() { return Letter(data ^ Letter::mask); }


    friend std::ostream &operator<< (std::ostream &os, const Letter &l) {
        return os << Codec::to_ext(l.data);
    }

    Holder data;
};

} /* namespace triegraph */

#endif /* __LETTER_H__ */
