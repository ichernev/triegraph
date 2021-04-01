#ifndef __DNA_LETTER_H__
#define __DNA_LETTER_H__

#include "alphabet/letter.h"

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


using DnaLetter = Letter<typename DnaLetterCodec::int_type, 4, DnaLetterCodec>;

struct DnaLetters {
    static constexpr auto A = DnaLetter(DnaLetter::Codec::to_int('A'));
    static constexpr auto C = DnaLetter(DnaLetter::Codec::to_int('C'));
    static constexpr auto G = DnaLetter(DnaLetter::Codec::to_int('G'));
    static constexpr auto T = DnaLetter(DnaLetter::Codec::to_int('T'));
    static constexpr auto EPS = DnaLetter(DnaLetter::num_options);
};

} /* namespace triegraph::dna */

#endif /* __DNA_LETTER_H__ */
