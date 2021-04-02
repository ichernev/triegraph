#include "alphabet/letter.h"
#include "alphabet/dna_letter.h"

#include "helper.h"

using namespace triegraph;
using dna::DnaLetters;
using dna::DnaLetter;

using TLetter = Letter<unsigned char, 4, CodecIdentity<char, unsigned char>>;
void get_tletter(TLetter t) {
    assert(true && "got tletter");
}
void get_char(char x) {
    assert(true && "got tletter");
}

int m = test::define_module(__FILE__, [] {

test::define_test("implicit_conversion", [] {
    get_tletter('a');
    get_char(TLetter('a'));
    TLetter t('a');
});

test::define_test("dna_letter", [] {
    assert(DnaLetters::A.data == 0 && "test A");
    assert(DnaLetters::C.data == 1 && "test C");
    assert(DnaLetters::G.data == 2 && "test G");
    assert(DnaLetters::T.data == 3 && "test T");

    assert(DnaLetters::A == DnaLetters::A);
});

test::define_test("dna_basic_props", [] {
    assert(DnaLetter::bits == 2);
});

});
