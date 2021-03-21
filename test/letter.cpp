#include "alphabet/letter.h"
#include "alphabet/dna_letter.h"

#include <assert.h>

#include <iostream>

using namespace triegraph;
using dna::DnaLetters;
using dna::DnaLetter;

using TLetter = Letter<unsigned char, 4, char, void, void>;
void get_tletter(TLetter t) {
    assert(true && "got tletter");
}
void get_char(char x) {
    assert(true && "got tletter");
}

void test_implicit_conversion() {
    get_tletter('a');
    get_char(TLetter('a'));
    TLetter t('a');
}

void test_dna_letter() {
    assert(DnaLetters::A.data == 0 && "test A");
    assert(DnaLetters::C.data == 1 && "test C");
    assert(DnaLetters::G.data == 2 && "test G");
    assert(DnaLetters::T.data == 3 && "test T");

    assert(DnaLetters::A == DnaLetters::A);
}

void test_dna_basic_props() {
    assert(DnaLetter::bits == 2);
}

int main() {
    std::cerr << "Running letter tests" << std::endl;
    test_implicit_conversion();
    test_dna_letter();
    test_dna_basic_props();
}
