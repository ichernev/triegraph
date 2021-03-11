#ifndef __DNA_LETTER_H__
#define __DNA_LETTER_H__

#include "letter.h"

namespace triegraph::dna {

template <typename Holder_, typename Human_ = char>
struct DnaEncoder {
    using Holder = Holder_;
    using Human = Human_;
    constexpr Holder operator()(Human letter) {
        switch (letter) {
            case 'A': case 'a':
                return 0;
            case 'C': case 'c':
                return 1;
            case 'G': case 'g':
                return 2;
            case 'T': case 't':
                return 3;
            case 'E':
                return 4;
            default:
                throw "invalid letter";
        }
    }
};

template <typename Holder_, typename Human_ = char>
struct DnaDecoder {
    using Holder = Holder_;
    using Human = Human_;
    constexpr Human operator()(Holder repr) {
        return "acgtE"[repr];
    }
};

using DnaLetter = Letter<u8, 4, char, DnaEncoder<u8>, DnaDecoder<u8>>;

struct DnaLetters {
    static constexpr auto A = DnaLetter(DnaLetter::Encoder()('A'));
    static constexpr auto C = DnaLetter(DnaLetter::Encoder()('C'));
    static constexpr auto G = DnaLetter(DnaLetter::Encoder()('G'));
    static constexpr auto T = DnaLetter(DnaLetter::Encoder()('T'));
    static constexpr auto EPS = DnaLetter(DnaLetter::num_options);
};

} /* namespace triegraph::dna */

#endif /* __DNA_LETTER_H__ */
