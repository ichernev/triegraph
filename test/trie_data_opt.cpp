#include "dna_config.h"
#include "manager.h"

#include <algorithm>

#include "helper.h"

template <bool allow_inner>
struct Cfg : public triegraph::dna::DnaConfig<0> {
    static constexpr bool triedata_advanced = true;
    static constexpr bool triedata_allow_inner = allow_inner;
};

int m = test::define_module(__FILE__, [] {

test::define_test("no_inner", [] {
    using TG = triegraph::Manager<Cfg<false>>;
    TG::init({ .add_reverse_complement = false, .trie_depth = 4 });

    auto g = TG::Graph::Builder({ .add_reverse_complement = false })
        .add_node(TG::Str("acgtacgt"), "s1")
        .build();
    auto lloc = TG::LetterLocData(g);

    auto td = TG::TrieData(std::vector<std::pair<TG::Kmer, TG::LetterLoc>> {
            { TG::Kmer::from_str("acgt"), 4 },
            { TG::Kmer::from_str("acgt"), 8 },
            { TG::Kmer::from_str("gtac"), 6 }
    }, lloc);

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
    using TG = triegraph::Manager<Cfg<true>>;
    TG::init({ .add_reverse_complement = false, .trie_depth = 4 });

    auto kmer_s = [](auto str) { return TG::Kmer::from_str(str); };
    auto g = TG::Graph::Builder({ .add_reverse_complement = false })
        .add_node(TG::Str("acgtacgt"), "s1")
        .build();
    auto lloc = TG::LetterLocData(g);

    auto td = TG::TrieData(std::vector<std::pair<TG::Kmer, TG::LetterLoc>> {
            { TG::Kmer::from_str("acgt"), 4 },
            { TG::Kmer::from_str("acgt"), 8 },
            { TG::Kmer::from_str("gtac"), 6 },
            { TG::Kmer::from_str("tt"), 2 } // pairs don't have to be accurate ...
    }, lloc);

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
