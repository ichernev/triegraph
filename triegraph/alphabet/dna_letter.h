// SPDX-License-Identifier: MPL-2.0
/*
 * Copyright (c) 2021, Iskren Chernev
 */

#ifndef __DNA_LETTER_H__
#define __DNA_LETTER_H__

#include "triegraph/alphabet/letter.h"

namespace triegraph::dna {

struct DnaLetterCodec {
    using ext_type = char;
    using int_type = u8;

    static constexpr int_type to_int(const ext_type &letter) {
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
                std::cerr << "got letter " << letter << std::endl;
                throw "invalid letter";
        }
    }

    static constexpr ext_type to_ext(const int_type &in) {
        return "acgtE"[in];
    }
};

struct DnaNLetterCodec {
    // Like DNA letter but supports N (ambiguous read)
    using ext_type = char;
    using int_type = u8;

    static constexpr int_type to_int(const ext_type &letter) {
        switch (letter) {
            case 'N': case 'n':
                return 4;
            case 'E':
                return 5;
            default:
                return DnaLetterCodec::to_int(letter);
        }
    }

    static constexpr ext_type to_ext(const int_type &in) {
        return "acgtnE"[in];
    }
};

using DnaLetter = Letter<typename DnaLetterCodec::int_type, 4, DnaLetterCodec>;
using DnaNLetter = Letter<typename DnaNLetterCodec::int_type, 5, DnaNLetterCodec>;

struct DnaLetters {
    static constexpr auto A = DnaLetter(DnaLetter::Codec::to_int('A'));
    static constexpr auto C = DnaLetter(DnaLetter::Codec::to_int('C'));
    static constexpr auto G = DnaLetter(DnaLetter::Codec::to_int('G'));
    static constexpr auto T = DnaLetter(DnaLetter::Codec::to_int('T'));
    static constexpr auto EPS = DnaLetter(DnaLetter::num_options);
};

struct DnaNLetters {
    static constexpr auto A = DnaNLetter(DnaNLetter::Codec::to_int('A'));
    static constexpr auto C = DnaNLetter(DnaNLetter::Codec::to_int('C'));
    static constexpr auto G = DnaNLetter(DnaNLetter::Codec::to_int('G'));
    static constexpr auto T = DnaNLetter(DnaNLetter::Codec::to_int('T'));
    static constexpr auto N = DnaNLetter(DnaNLetter::Codec::to_int('N'));
    static constexpr auto EPS = DnaNLetter(DnaNLetter::num_options);
};

} /* namespace triegraph::dna */

#endif /* __DNA_LETTER_H__ */
