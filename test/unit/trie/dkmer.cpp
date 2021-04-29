#include "trie/dkmer.h"
#include "alphabet/dna_letter.h"

#include "testlib/test.h"
// #include <assert.h>
// #include <iostream>
#include <iterator>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <span>

using namespace triegraph;
using dna::DnaLetter;
using dna::DnaLetters;

using DnaKmer = DKmer<DnaLetter, u32>;

int m = test::define_module(__FILE__, [] {

test::define_test("push", [] {
    DnaKmer::set_settings({ .trie_depth = 15 });
    DnaKmer kmer = DnaKmer::empty();

    assert(!kmer.is_complete());
    assert(kmer.get_len() == 0);

    for (DnaKmer::klen_type i = 0; i < DnaKmer::K; ++i) {
        assert(kmer.get_len() == i);
        assert(!kmer.is_complete());
        kmer.push(DnaLetters::A);
    }

    assert(kmer.is_complete());
    assert(kmer.get_len() == DnaKmer::K);
});

test::define_test("pop", [] {
    DnaKmer::set_settings({ .trie_depth = 15 });
    auto kmer = DnaKmer::empty();
    for (DnaKmer::klen_type i = 0; i < DnaKmer::K; ++i) {
        kmer.push(DnaLetters::A);
    }

    assert(kmer.is_complete());
    for (DnaKmer::klen_type i = DnaKmer::K - 1; i < DnaKmer::K; --i) {
        kmer.pop();
        assert(kmer.get_len() == i);
    }

    assert(kmer.get_len() == 0);
});

test::define_test("iter", [] {
    DnaKmer::set_settings({ .trie_depth = 15 });
    std::vector<DnaLetter> seq;
    for (DnaKmer::klen_type i = 0; i < DnaKmer::K; ++i) {
        seq.push_back(rand() % DnaLetter::num_options);
    }

    auto kmer = DnaKmer::empty();
    for (auto s : seq) {
        kmer.push(s);
        // std::cerr << kmer << std::endl;
        assert(std::equal(kmer.begin(), kmer.end(), seq.begin()));
    }

    for (DnaKmer::klen_type i = 0; i < DnaKmer::K; ++i) {
        // std::cerr << kmer << std::endl;
        assert(std::equal(kmer.end() - i, kmer.end(), seq.begin()));
        assert(std::equal(kmer.begin(), kmer.end() - i, seq.begin() + i));
        kmer.push(seq[i]);
    }

    // std::cerr << std::hex << DnaKmer::_kmer_mask(1) << std::dec << std::endl;
    // std::cerr << std::hex << kmer.data << std::dec << std::endl;
    while (kmer.get_len() > 0) {
        // std::cerr << "popping at len " << kmer.get_len() << std::endl;
        kmer.pop();
        // std::cerr << std::hex << kmer.data << std::dec << std::endl;
        // std::cerr << kmer << std::endl;
        assert(std::equal(kmer.begin(), kmer.end(), seq.begin()));
    }
});

test::define_test("concept", [] {
    DnaKmer::set_settings({ .trie_depth = 15 });
    auto kmer = DnaKmer::empty();
    auto func = [](std::random_access_iterator auto x) {};
    func(kmer.begin());
});

test::define_test("on_mask", [] {
    // basically same as test_iter test, but also check for on_mask
    u64 on_mask = u64(1) << 63;
    using DnaKmer_u = DKmer<DnaLetter, u64>;
    DnaKmer_u::set_settings({ .trie_depth = 31, .on_mask = on_mask });

    std::vector<DnaLetter> seq;
    for (DnaKmer_u::klen_type i = 0; i < DnaKmer_u::K; ++i) {
        seq.push_back(rand() % DnaLetter::num_options);
    }

    auto kmer = DnaKmer_u::empty();
    assert(kmer.data & on_mask);
    for (auto s : seq) {
        // std::cerr << kmer << std::endl;
        kmer.push(s);
        assert(kmer.data & on_mask);
        assert(std::equal(kmer.begin(), kmer.end(), seq.begin()));
    }

    for (DnaKmer_u::klen_type i = 0; i < DnaKmer_u::K; ++i) {
        // std::cerr << kmer << std::endl;
        assert(std::equal(kmer.end() - i, kmer.end(), seq.begin()));
        assert(std::equal(kmer.begin(), kmer.end() - i, seq.begin() + i));
        kmer.push(seq[i]);
        assert(kmer.data & on_mask);
    }

    while (kmer.get_len() > 0) {
        kmer.pop();
        // std::cerr << kmer << std::endl;
        assert(std::equal(kmer.begin(), kmer.end(), seq.begin()));
        assert(kmer.data & on_mask);
    }
});

test::define_test("map_friendly", [] {
    u64 on_mask = u64(1) << 63;
    using DnaKmer_u = DKmer<DnaLetter, u64>;
    DnaKmer_u::set_settings({ .trie_depth = 31, .on_mask = on_mask });

    std::unordered_multimap<DnaKmer, u32> a1;
    // std::unordered_multimap<u32, DnaKmer> b1;
    std::unordered_set<DnaKmer> c1;
    a1.emplace(DnaKmer::empty(), 0);
    c1.emplace(DnaKmer::empty());

    std::unordered_multimap<DnaKmer_u, u32> a2;
    // std::unordered_multimap<u32, DnaKmer_u> b2;
    std::unordered_set<DnaKmer_u> c2;
    a2.emplace(DnaKmer_u::empty(), 0);
    c2.emplace(DnaKmer_u::empty());
});

test::define_test("to_from_str", [] {
    using Kmer4 = DKmer<DnaLetter, u64>;
    Kmer4::set_settings({ .trie_depth = 4 });

    assert(DnaKmer::from_str("acgt").to_str() == "acgt");
    assert(Kmer4::from_str("acgta").to_str() == "cgta");
});

test::define_test("cmp", [] {
    DnaKmer::set_settings({ .trie_depth = 15 });
    assert(DnaKmer::from_str("acgt") < DnaKmer::from_str("actt"));
    assert(DnaKmer::from_str("acgt") == DnaKmer::from_str("acgt"));
});

test::define_test("indexing", [] {
    DnaKmer::set_settings({ .trie_depth = 15 });
    auto k = DnaKmer::from_str("acgt");
    assert(k[0] == 0);
    assert(k[1] == 1);
    assert(k[2] == 2);
    assert(k[3] == 3);
});

test::define_test("static_arrays", [] {
    {
        using Letter = triegraph::Letter<u8, 4>;
        using Kmer = triegraph::DKmer<Letter, u32>;
        Kmer::set_settings({ .trie_depth = 4, .on_mask = u32(1) << 31 });

        // std::ranges::copy(Kmer::beg, std::ostream_iterator<u32>(std::cerr, " ")); std::cerr << std::endl;
        assert(std::ranges::equal(std::vector<u32> {0, 1, 5, 21, 85}, std::span(Kmer::beg.begin(), 5)));
        assert(std::ranges::equal(std::vector<u32> {1, 4, 16, 64}, std::span(Kmer::lvl_size.begin(), 4)));
    }

    {
        using Letter = triegraph::Letter<u8, 2>;
        using Kmer = triegraph::DKmer<Letter, u32>;
        Kmer::set_settings({ .trie_depth = 3, .on_mask = u32(1) << 31 });

        // std::ranges::copy(Kmer::beg, std::ostream_iterator<u32>(std::cerr, " ")); std::cerr << std::endl;
        assert(std::ranges::equal(std::vector<u32> {0, 1, 3, 7}, std::span(Kmer::beg.begin(), 4)));
        assert(std::ranges::equal(std::vector<u32> {1, 2, 4}, std::span(Kmer::lvl_size.begin(), 3)));
    }
});

test::define_test("compressed_2x2", [] {
    using Letter = triegraph::Letter<u8, 2>;
    using Kmer = triegraph::DKmer<Letter, u32>;
    Kmer::set_settings({ .trie_depth = 2, .on_mask = u32(1) << 31 });

    auto fc = [](int i) { return Kmer::from_compressed(i); };

    assert(Kmer::lvl_size[Kmer::K] == 4);
    assert(Kmer::beg[Kmer::K] == 3);
    assert(Kmer::beg[Kmer::K+1] == 7);

    assert(fc(0).size() == 0);
    assert(fc(1).size() == 1 && fc(1)[0] == 0);
    assert(fc(2).size() == 1 && fc(2)[0] == 1);
    assert(fc(3).size() == 2 && fc(3)[0] == 0 && fc(3)[1] == 0);
    assert(fc(4).size() == 2 && fc(4)[0] == 0 && fc(4)[1] == 1);
    assert(fc(5).size() == 2 && fc(5)[0] == 1 && fc(5)[1] == 0);
    assert(fc(6).size() == 2 && fc(6)[0] == 1 && fc(6)[1] == 1);

    for (typename Kmer::Holder i = 0; i < Kmer::beg[Kmer::K+1]; ++i) {
        assert(fc(i).compress() == i);
    }
    for (typename Kmer::Holder i = 0; i < Kmer::lvl_size[Kmer::K]; ++i) {
        assert(Kmer::from_compressed_leaf(i).compress_leaf() == i);
    }
});

test::define_test("compressed_4x2", [] {
    using Letter = triegraph::Letter<u8, 4>;
    using Kmer = triegraph::DKmer<Letter, u32>;
    Kmer::set_settings({ .trie_depth = 2, .on_mask = u32(1) << 31 });

    auto fc = [](int i) { return Kmer::from_compressed(i); };

    assert(Kmer::lvl_size[Kmer::K] == 16);
    assert(Kmer::beg[Kmer::K] == 5);
    assert(Kmer::beg[Kmer::K+1] == 21);

    assert(fc(0).size() == 0);
    assert(fc(1).size() == 1 && fc(1)[0] == 0);
    assert(fc(2).size() == 1 && fc(2)[0] == 1);
    assert(fc(3).size() == 1 && fc(3)[0] == 2);
    assert(fc(4).size() == 1 && fc(4)[0] == 3);
    assert(fc(5).size() == 2 && fc(5)[0] == 0 && fc(5)[1] == 0);
    assert(fc(6).size() == 2 && fc(6)[0] == 0 && fc(6)[1] == 1);
    assert(fc(20).size() == 2 && fc(20)[0] == 3 && fc(20)[1] == 3);

    for (typename Kmer::Holder i = 0; i < Kmer::beg[Kmer::K+1]; ++i) {
        assert(fc(i).compress() == i);
    }
    for (typename Kmer::Holder i = 0; i < Kmer::lvl_size[Kmer::K]; ++i) {
        assert(Kmer::from_compressed_leaf(i).compress_leaf() == i);
    }
});

});
