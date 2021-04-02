#include "trie/kmer.h"
#include "alphabet/dna_letter.h"

#include "helper.h"
// #include <assert.h>
// #include <iostream>
#include <iterator>
#include <string>
#include <unordered_map>
#include <unordered_set>

using namespace triegraph;
using dna::DnaLetter;
using dna::DnaLetters;

using DnaKmer31 = Kmer<DnaLetter, u64, 31>;

int m = test::define_module(__FILE__, [] {

test::define_test("sanity", [] {
// static void test_sanity() {
    assert(DnaKmer31::K == 31);
    assert(DnaKmer31::MAX_K == 31);
});
test::define_test("push", [] {
// static void test_push() {
    DnaKmer31 kmer = DnaKmer31::empty();

    assert(!kmer.is_complete());
    assert(kmer.get_len() == 0);

    for (DnaKmer31::klen_type i = 0; i < DnaKmer31::K; ++i) {
        assert(kmer.get_len() == i);
        assert(!kmer.is_complete());
        kmer.push(DnaLetters::A);
    }

    assert(kmer.is_complete());
    assert(kmer.get_len() == DnaKmer31::K);
});
test::define_test("pop", [] {
// static void test_pop() {
    auto kmer = DnaKmer31::empty();
    for (DnaKmer31::klen_type i = 0; i < DnaKmer31::K; ++i) {
        kmer.push(DnaLetters::A);
    }

    assert(kmer.is_complete());
    for (DnaKmer31::klen_type i = DnaKmer31::K - 1; i < DnaKmer31::K; --i) {
        kmer.pop();
        assert(kmer.get_len() == i);
    }

    assert(kmer.get_len() == 0);
});
test::define_test("iter", [] {
// static void test_iter() {
    std::vector<DnaLetter> seq;
    for (DnaKmer31::klen_type i = 0; i < DnaKmer31::K; ++i) {
        seq.push_back(rand() % DnaLetter::num_options);
    }

    auto kmer = DnaKmer31::empty();
    for (auto s : seq) {
        kmer.push(s);
        // std::cerr << kmer << std::endl;
        assert(std::equal(kmer.begin(), kmer.end(), seq.begin()));
    }

    for (DnaKmer31::klen_type i = 0; i < DnaKmer31::K; ++i) {
        // std::cerr << kmer << std::endl;
        assert(std::equal(kmer.end() - i, kmer.end(), seq.begin()));
        assert(std::equal(kmer.begin(), kmer.end() - i, seq.begin() + i));
        kmer.push(seq[i]);
    }

    // std::cerr << std::hex << DnaKmer31::_kmer_mask(1) << std::dec << std::endl;
    // std::cerr << std::hex << kmer.data << std::dec << std::endl;
    while (kmer.get_len() > 0) {
        // std::cerr << "popping at len " << kmer.get_len() << std::endl;
        kmer.pop();
        // std::cerr << std::hex << kmer.data << std::dec << std::endl;
        // std::cerr << kmer << std::endl;
        assert(std::equal(kmer.begin(), kmer.end(), seq.begin()));
    }
});

// template<std::random_access_iterator IT>
// void func(IT x) {}
test::define_test("concep", [] {
// static void test_concep() {

    auto func = [](std::random_access_iterator auto x) {};
    auto kmer = DnaKmer31::empty();
    func(kmer.begin());
});
test::define_test("on_mask", [] {
// static void test_on_mask() {
    // basically same as test_iter test, but also check for on_mask
    constexpr u64 on_mask = u64(1) << 63;
    using DnaKmer31_u = Kmer<DnaLetter, u64, 31, on_mask>;

    std::vector<DnaLetter> seq;
    for (DnaKmer31_u::klen_type i = 0; i < DnaKmer31_u::K; ++i) {
        seq.push_back(rand() % DnaLetter::num_options);
    }

    auto kmer = DnaKmer31_u::empty();
    assert(kmer.data & on_mask);
    for (auto s : seq) {
        kmer.push(s);
        assert(kmer.data & on_mask);
        assert(std::equal(kmer.begin(), kmer.end(), seq.begin()));
    }

    for (DnaKmer31::klen_type i = 0; i < DnaKmer31::K; ++i) {
        assert(std::equal(kmer.end() - i, kmer.end(), seq.begin()));
        assert(std::equal(kmer.begin(), kmer.end() - i, seq.begin() + i));
        kmer.push(seq[i]);
        assert(kmer.data & on_mask);
    }

    while (kmer.get_len() > 0) {
        kmer.pop();
        assert(std::equal(kmer.begin(), kmer.end(), seq.begin()));
        assert(kmer.data & on_mask);
    }
});
test::define_test("map_friendly", [] {
// static void test_map_friendly() {
    constexpr u64 on_mask = u64(1) << 63;
    using DnaKmer31_u = Kmer<DnaLetter, u64, 31, on_mask>;

    std::unordered_multimap<DnaKmer31, u32> a1;
    // std::unordered_multimap<u32, DnaKmer31> b1;
    std::unordered_set<DnaKmer31> c1;
    a1.emplace(DnaKmer31::empty(), 0);
    c1.emplace(DnaKmer31::empty());

    std::unordered_multimap<DnaKmer31_u, u32> a2;
    // std::unordered_multimap<u32, DnaKmer31_u> b2;
    std::unordered_set<DnaKmer31_u> c2;
    a2.emplace(DnaKmer31_u::empty(), 0);
    c2.emplace(DnaKmer31_u::empty());
});
test::define_test("to_from_str", [] {
// static void test_to_from_str() {
    assert(DnaKmer31::from_str("acgt").to_str() == "acgt");
    using Kmer4 = Kmer<DnaLetter, u64, 4>;
    assert(Kmer4::from_str("acgta").to_str() == "cgta");
});
test::define_test("cmp", [] {
// static void test_cmp() {
    assert(DnaKmer31::from_str("acgt") < DnaKmer31::from_str("actt"));
    assert(DnaKmer31::from_str("acgt") == DnaKmer31::from_str("acgt"));
});
test::define_test("indexing", [] {
// static void test_indexing() {
    auto k = DnaKmer31::from_str("acgt");
    assert(k[0] == 0);
    assert(k[1] == 1);
    assert(k[2] == 2);
    assert(k[3] == 3);
});
test::define_test("level_beg", [] {
// static void test_level_beg() {
    {
        using Letter = triegraph::Letter<u8, 4>;
        using Kmer = triegraph::Kmer<Letter, u32, 4, u32(1) << 31>;

        std::vector<u64> beg = {0, 1, 5, 21, 85};
        assert(std::equal(Kmer::beg.begin(), Kmer::beg.end(), beg.begin()));
    }

    {
        using Letter = triegraph::Letter<u8, 2>;
        using Kmer = triegraph::Kmer<Letter, u32, 2, u32(1) << 31>;

        std::vector<u64> beg = {0, 1, 3};
        assert(std::equal(Kmer::beg.begin(), Kmer::beg.end(), beg.begin()));
    }
});
test::define_test("compressed_2x2", [] {
// static void test_compressed_2x2() {
    using Letter = triegraph::Letter<u8, 2>;
    using Kmer = triegraph::Kmer<Letter, u32, 2, u32(1) << 31>;

    auto fc = [](int i) { return Kmer::from_compressed(i); };

    assert(Kmer::NUM_LEAFS == 4);
    assert(Kmer::NUM_COMPRESSED == 7);
    assert(Kmer::TrieElems<2>::value == 3);

    assert(fc(0).size() == 0);
    assert(fc(1).size() == 1 && fc(1)[0] == 0);
    assert(fc(2).size() == 1 && fc(2)[0] == 1);
    assert(fc(3).size() == 2 && fc(3)[0] == 0 && fc(3)[1] == 0);
    assert(fc(4).size() == 2 && fc(4)[0] == 0 && fc(4)[1] == 1);
    assert(fc(5).size() == 2 && fc(5)[0] == 1 && fc(5)[1] == 0);
    assert(fc(6).size() == 2 && fc(6)[0] == 1 && fc(6)[1] == 1);

    for (typename Kmer::Holder i = 0; i < Kmer::NUM_COMPRESSED; ++i) {
        assert(fc(i).compress() == i);
    }
    for (typename Kmer::Holder i = 0; i < Kmer::NUM_LEAFS; ++i) {
        assert(Kmer::from_compressed_leaf(i).compress_leaf() == i);
    }
});
test::define_test("compressed_4x2", [] {
// static void test_compressed_4x2() {
    using Letter = triegraph::Letter<u8, 4>;
    using Kmer = triegraph::Kmer<Letter, u32, 2, u32(1) << 31>;

    auto fc = [](int i) { return Kmer::from_compressed(i); };

    assert(Kmer::NUM_LEAFS == 16);
    assert(Kmer::NUM_COMPRESSED == 21);
    assert(Kmer::TrieElems<2>::value == 5);

    assert(fc(0).size() == 0);
    assert(fc(1).size() == 1 && fc(1)[0] == 0);
    assert(fc(2).size() == 1 && fc(2)[0] == 1);
    assert(fc(3).size() == 1 && fc(3)[0] == 2);
    assert(fc(4).size() == 1 && fc(4)[0] == 3);
    assert(fc(5).size() == 2 && fc(5)[0] == 0 && fc(5)[1] == 0);
    assert(fc(6).size() == 2 && fc(6)[0] == 0 && fc(6)[1] == 1);
    assert(fc(20).size() == 2 && fc(20)[0] == 3 && fc(20)[1] == 3);

    for (typename Kmer::Holder i = 0; i < Kmer::NUM_COMPRESSED; ++i) {
        assert(fc(i).compress() == i);
    }
    for (typename Kmer::Holder i = 0; i < Kmer::NUM_LEAFS; ++i) {
        assert(Kmer::from_compressed_leaf(i).compress_leaf() == i);
    }
});

});

// int main() {
//     test_sanity();
//     test_push();
//     test_pop();
//     test_iter();
//     test_concep();
//     test_on_mask();
//     test_map_friendly();
//     test_to_from_str();
//     test_cmp();
//     test_level_beg();
//     test_indexing();
//     test_compressed_2x2();
//     test_compressed_4x2();

//     return 0;
// }
