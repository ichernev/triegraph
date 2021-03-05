#include "kmer.h"
#include "dna_letter.h"

#include <assert.h>
#include <iostream>
#include <iterator>
// #include <format>
#include <string>

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

int main() {
    test_sanity();
    test_push();
    test_pop();
    test_iter();
    test_concep();

    return 0;
}
