#ifndef __DNA_LETTER_H__
#define __DNA_LETTER_H__

#include "letter.h"

template <typename Holder_, typename Human_ = char>
struct DnaMapper {
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
            default:
                throw "invalid letter";
        }
    }
};

template <typename Holder_, typename Human_ = char>
struct DnaUnmapper {
    using Holder = Holder_;
    using Human = Human_;
    constexpr Human operator()(Holder repr) {
        return "acgt"[repr];
    }
};

using DnaLetter = Letter<u8, 4, char, DnaMapper<u8>, DnaUnmapper<u8>>;

struct DnaLetters {
    static constexpr auto A = DnaLetter(DnaLetter::Mapper()('A'));
    static constexpr auto C = DnaLetter(DnaLetter::Mapper()('C'));
    static constexpr auto G = DnaLetter(DnaLetter::Mapper()('G'));
    static constexpr auto T = DnaLetter(DnaLetter::Mapper()('T'));
};

#endif /* __DNA_LETTER_H__ */
