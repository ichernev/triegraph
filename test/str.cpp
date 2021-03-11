// read str, check actual bits
// test comparing two views, aligned unaligned
// check speed of comparing aligned vs unaligned views
// iterator
//
// * write fast_search when not aligned
// * make fast search work a byte at a time
// * revcomp copy
// * revcomp view

#include "dna_letter.h"
#include "str.h"

#include <assert.h>

#include <iostream>
#include <sstream>

using namespace triegraph;
using dna::DnaLetter;
using dna::DnaLetters;

using DnaStr = Str<dna::DnaLetter, u32>;

static void test_create() {
    auto dna_str = DnaStr("acgt");
    assert(dna_str.length == 4);
    assert(dna_str.capacity == 1);
    assert(dna_str[0] == DnaLetters::A);
    assert(dna_str[1] == DnaLetters::C);
    assert(dna_str[2] == DnaLetters::G);
    assert(dna_str[3] == DnaLetters::T);
    assert(dna_str.data[0] == 0xe4); // 1110 0100
}

static void test_view() {
    auto dna_str = DnaStr("acgt");
    auto dna_view = dna_str.get_view(2);
    assert(dna_view.length == 2);
    assert(dna_view.offset == 2);
    assert(dna_view[0] == DnaLetters::G);
    assert(dna_view[1] == DnaLetters::T);
}

static void test_view_compare() {
    try {
        auto str = DnaStr("aaacaaat");
        auto v1 = str.get_view(0, 4);
        auto v2 = str.get_view(4, 4);
        assert(v1.fast_match(v2) == 3);
    } catch (const char *x) {
        std::cerr << "got exception " << x << std::endl;
    }
}

static void test_stream_input() {
    std::istringstream io("acgt");
    DnaStr s;

    io >> s;
    assert(s.length == 4);
    assert(s[1] == DnaLetters::C);
}

static void test_stream_output() {
    std::ostringstream io;
    DnaStr s("acgt");

    io << s;
    assert(io.str() == "acgt");

    io.str("");
    io << s.get_view(1, 2);
    assert(io.str() == "cg");
}

static void test_str_iterator() {
    DnaStr s("acgt");
    auto it = s.begin();
    assert(*it == DnaLetters::A); ++it;
    assert(*it == DnaLetters::C); ++it;
    assert(*it == DnaLetters::G); ++it;
    assert(*it == DnaLetters::T); ++it;
    assert(it == s.end());
}

static void test_view_iterator() {
    DnaStr s("acgt");
    auto v = s.get_view(1, 2);
    auto it = v.begin();
    assert(*it == DnaLetters::C); ++it;
    assert(*it == DnaLetters::G); ++it;
    assert(it == v.end());
}

static void test_move() {
    DnaStr s("acgt");

    DnaStr s2 = std::move(s);
    s = std::move(s2);

    assert(s.length == 4);
}

int main() {
    std::cerr << "Running str tests" << std::endl;

    test_create();
    test_view();
    test_view_compare();
    test_stream_input();
    test_stream_output();
    test_str_iterator();
    test_view_iterator();
    test_move();

    return 0;
}
