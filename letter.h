#ifndef __LETTER_H__
#define __LETTER_H__

#include "util.h"

using u64 = unsigned long long;
template <typename Holder_, u64 options_,
         typename Human_, typename Mapper_, typename Unmapper_>
struct Letter {
    using Holder = Holder_;
    using Human = Human_;
    using Mapper = Mapper_;
    using Unmapper = Unmapper_;
    static constexpr u64 num_options = options_;
    static constexpr int bits = log2_ceil(num_options);
    static constexpr Holder mask = (1 << bits) - 1;

    constexpr Letter(Holder h) : data(h) { }
    // Letter(&&)
    // operator=(&)
    // operator=(&&)

    constexpr operator Holder() {
        return data;
    }
    bool operator == (const Letter &other) const { return data == other.data; }
    bool operator != (const Letter &other) const { return data != other.data; }

    Holder data;
};

#endif /* __LETTER_H__ */
