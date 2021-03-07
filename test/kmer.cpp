#include "kmer.h"
#include "dna_letter.h"

#include <assert.h>
#include <iostream>
#include <iterator>
#include <string>
#include <unordered_map>
#include <unordered_set>

using DnaKmer31 = Kmer<DnaLetter, u64, 31>;

static void test_sanity() {
    assert(DnaKmer31::K == 31);
    assert(DnaKmer31::MAX_K == 31);
}

static void test_push() {
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
}

static void test_pop() {
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
}

static void test_iter() {
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
}

template<std::random_access_iterator IT>
void func(IT x) {}
static void test_concep() {

    auto kmer = DnaKmer31::empty();
    func(kmer.begin());
}

static void test_on_mask() {
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
}

static void test_map_friendly() {
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
}

static void test_to_from_str() {
    assert(DnaKmer31::from_str("acgt").to_str() == "acgt");
    using Kmer4 = Kmer<DnaLetter, u64, 4>;
    assert(Kmer4::from_str("acgta").to_str() == "cgta");
}

int main() {
    test_sanity();
    test_push();
    test_pop();
    test_iter();
    test_concep();
    test_on_mask();
    test_map_friendly();
    test_to_from_str();

    return 0;
}
