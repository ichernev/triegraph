#ifndef __LETTER_H__
#define __LETTER_H__

#include "util.h"

#include <type_traits>
#include <ostream>

namespace triegraph {

template <typename Holder_, u64 options_,
         typename Human_, typename Encoder_, typename Decoder_>
struct Letter {
    using Holder = Holder_;
    using Human = Human_;
    using Encoder = Encoder_;
    using Decoder = Decoder_;
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
    bool operator == (const Letter &other) const  = default;
    auto operator <=> (const Letter &other) const = default;

    Letter rev_comp() {
        return Letter(data ^ Letter::mask);
    }

    friend std::ostream &operator<< (std::ostream &os, const Letter &l) {
        return os << Decoder()(l.data);
    }

    Holder data;
};

} /* namespace triegraph */

#endif /* __LETTER_H__ */
