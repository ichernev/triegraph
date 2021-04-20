// read str, check actual bits
// test comparing two views, aligned unaligned
// check speed of comparing aligned vs unaligned views
// iterator
//
// * write fast_search when not aligned
// * make fast search work a byte at a time
// * revcomp copy
// * revcomp view

#include "alphabet/dna_letter.h"
#include "alphabet/str.h"

#include <sstream>

#include "testlib/test.h"

using namespace triegraph;
using dna::DnaLetter;
using dna::DnaLetters;

using DnaStr = Str<dna::DnaLetter, u32>;

int m = test::define_module(__FILE__, [] {

test::define_test("create", [] {
    auto dna_str = DnaStr("acgt");
    assert(dna_str.length == 4);
    assert(dna_str.capacity == 1);
    assert(dna_str[0] == DnaLetters::A);
    assert(dna_str[1] == DnaLetters::C);
    assert(dna_str[2] == DnaLetters::G);
    assert(dna_str[3] == DnaLetters::T);
    assert(dna_str.data[0] == 0xe4); // 1110 0100
});

test::define_test("view", [] {
    auto dna_str = DnaStr("acgt");
    auto dna_view = dna_str.get_view(2);
    assert(dna_view.length == 2);
    assert(dna_view.offset == 2);
    assert(dna_view[0] == DnaLetters::G);
    assert(dna_view[1] == DnaLetters::T);
});

test::define_test("view_compare", [] {
    try {
        auto str = DnaStr("aaacaaat");
        auto v1 = str.get_view(0, 4);
        auto v2 = str.get_view(4, 4);
        assert(v1.fast_match(v2) == 3);
    } catch (const char *x) {
        std::cerr << "got exception " << x << std::endl;
    }
});

test::define_test("stream_input", [] {
    std::istringstream io("acgt");
    DnaStr s;

    io >> s;
    assert(s.length == 4);
    assert(s[1] == DnaLetters::C);
});

test::define_test("stream_output", [] {
    std::ostringstream io;
    DnaStr s("acgt");

    io << s;
    assert(io.str() == "acgt");

    io.str("");
    io << s.get_view(1, 2);
    assert(io.str() == "cg");
});

test::define_test("str_iterator", [] {
    DnaStr s("acgt");
    auto it = s.begin();
    assert(*it == DnaLetters::A); ++it;
    assert(*it == DnaLetters::C); ++it;
    assert(*it == DnaLetters::G); ++it;
    assert(*it == DnaLetters::T); ++it;
    assert(it == s.end());
});

test::define_test("view_iterator", [] {
    DnaStr s("acgt");
    auto v = s.get_view(1, 2);
    auto it = v.begin();
    assert(*it == DnaLetters::C); ++it;
    assert(*it == DnaLetters::G); ++it;
    assert(it == v.end());
});

test::define_test("move", [] {
    DnaStr s("acgt");

    DnaStr s2 = std::move(s);
    s = std::move(s2);

    assert(s.length == 4);
});

test::define_test("revcomp", [] {
    std::string s;
    constexpr u32 size = 512;
    srand(0);
    for (u32 i = 0; i < size; ++i) {
        s.push_back(DnaLetter::Codec::to_ext(rand() % DnaLetter::num_options));
    }
    DnaStr ds(s);
    DnaStr rc = ds.rev_comp();

    assert(ds.size() == rc.size());
    assert(ds.capacity == rc.capacity);
    assert(rc.size() == size);
    for (u32 i = 0; i < size; ++i) {
        assert(rc[i] == ds[size-1-i].rev_comp());
        assert(rc[size-1-i].rev_comp() == ds[i]);
    }
    u32 i = 0;
    for (Letter l : rc) {
        assert(l == ds[size - ++i].rev_comp());
    }
});

});
