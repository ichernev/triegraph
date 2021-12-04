// SPDX-License-Identifier: MPL-2.0
/*
 * Copyright (c) 2021, Iskren Chernev
 */

#include "dna_config.h"
#include "manager.h"

#include <algorithm>

#include "testlib/test.h"

// template <bool allow_inner>
// struct Cfg : public triegraph::dna::DnaConfig<0, true, false>;

int m = test::define_module(__FILE__, [] {

test::define_test("no_inner", [] {
    using TG = triegraph::Manager<triegraph::dna::DnaConfig<0>>;
    TG::kmer_set_depth(4);

    auto g = TG::Graph::Builder({ .add_reverse_complement = false })
        .add_node(TG::Str("acgtacgt"), "s1")
        .build();
    auto lloc = TG::LetterLocData(g);

    auto vp = TG::VectorPairs {};
    auto vpi = TG::make_pairs_inserter(vp, TG::PairsVariantCompressed {});
    std::ranges::copy(std::vector<std::pair<TG::Kmer, TG::LetterLoc>> {
            { TG::Kmer::from_str("acgt"), 4 },
            { TG::Kmer::from_str("acgt"), 8 },
            { TG::Kmer::from_str("gtac"), 6 }
    }, std::back_inserter(vpi));
    auto td = TG::TrieData(vp, lloc);

    assert(test::equal_sorted(
                td.t2g_values_for(TG::Kmer::from_str("acgt")),
                std::vector<TG::LetterLoc> { 4, 8 }));
    assert(test::equal_sorted(
                td.t2g_values_for(TG::Kmer::from_str("gtac")),
                std::vector<TG::LetterLoc> { 6 }));
    assert(test::equal_sorted(
                td.t2g_values_for(TG::Kmer::from_str("acgg")),
                std::vector<TG::LetterLoc> { }));
});

test::define_test("with_inner", [] {
    using triegraph::dna::CfgFlags;
    using TG = triegraph::Manager<triegraph::dna::DnaConfig<0, CfgFlags::ALLOW_INNER_KMER>>;
    TG::kmer_set_depth(4);

    auto kmer_s = [](auto str) { return TG::Kmer::from_str(str); };
    auto g = TG::Graph::Builder({ .add_reverse_complement = false })
        .add_node(TG::Str("acgtacgt"), "s1")
        .build();
    auto lloc = TG::LetterLocData(g);

    auto vp = TG::VectorPairs {};
    auto vpi = TG::make_pairs_inserter(vp, TG::PairsVariantCompressed {});
    std::ranges::copy(std::vector<std::pair<TG::Kmer, TG::LetterLoc>> {
            { TG::Kmer::from_str("acgt"), 4 },
            { TG::Kmer::from_str("acgt"), 8 },
            { TG::Kmer::from_str("gtac"), 6 },
            { TG::Kmer::from_str("tt"), 2 } // pairs don't have to be accurate ...
    }, std::back_inserter(vpi));
    auto td = TG::TrieData(vp, lloc);

    using vec_l = std::vector<TG::LetterLoc>;
    assert(test::equal_sorted(td.t2g_values_for(kmer_s("acgt")), vec_l { 4, 8 }));
    assert(test::equal_sorted(td.t2g_values_for(kmer_s("gtac")), vec_l { 6 }));
    assert(test::equal_sorted(td.t2g_values_for(kmer_s("acgg")), vec_l { }));
    assert(test::equal_sorted(td.t2g_values_for(kmer_s("tt")),   vec_l { 2 }));
    assert(test::equal_sorted(td.t2g_values_for(kmer_s("tta")),  vec_l { }));

    assert( td.t2g_contains(kmer_s("acgt")));
    assert( td.t2g_contains(kmer_s("gtac")));
    assert(!td.t2g_contains(kmer_s("acgg")));
    assert( td.t2g_contains(kmer_s("tt")));
    assert(!td.t2g_contains(kmer_s("tta")));
    assert(!td.t2g_contains(kmer_s("tttt")));
    assert(!td.t2g_contains(kmer_s("acg")));

    assert( td.trie_contains(kmer_s("acgt")));
    assert( td.trie_contains(kmer_s("gtac")));
    assert(!td.trie_contains(kmer_s("acgg")));
    assert( td.trie_contains(kmer_s("tt")));
    assert(!td.trie_contains(kmer_s("tta")));
    assert(!td.trie_contains(kmer_s("tttt")));
    assert( td.trie_contains(kmer_s("acg")));
});

});
