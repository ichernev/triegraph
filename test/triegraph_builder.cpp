#include "util.h"
#include "dna_letter.h"
#include "manager.h"

#include <assert.h>
#include <iostream>
#include <algorithm>

using namespace triegraph;

struct Cfg {
    using Letter = dna::DnaLetter;
    using StrHolder = u32;
    using NodeLoc = u32;
    using NodeLen = u32;
    using EdgeLoc = u32;
    using LetterLoc = u32;
    using KmerHolder = u64;
    static constexpr u64 KmerLen = 4;
    static constexpr KmerHolder on_mask = KmerHolder(1) << 63;
};

using MGR = Manager<Cfg>;

// using Graph = RgfaGraph<DnaStr, u32>;
// using NPos = NodePos<u32>;
// using LLData = LetterLocData<NPos, Graph>;
// constexpr u64 on_mask = u64(1) << 63;
// using DnaKmer = Kmer<dna::DnaLetter, u64, 4, on_mask>;
// using TData = TrieData<DnaKmer, u32>;
// using LetterLoc = TData::LetterLoc;
// using TrieGraphData = TrieGraphData<Graph, LLData, TData>;
// using TB = TriegraphBuilder<TrieGraphData>;

static std::vector<MGR::LetterLoc> trie2graph(const MGR::TrieGraphData &tg, MGR::Kmer kmer) {
    auto pos_it = tg.trie_data.t2g_values_for(kmer);
    std::vector<MGR::LetterLoc> pos;
    std::ranges::copy(pos_it, std::back_inserter(pos));
    std::sort(pos.begin(), pos.end());
    return pos;
}

static void test_tiny_linear_graph() {
    MGR::Graph::Builder b;
    auto g = b.add_node(MGR::Str("acgtacgtac"), "s1").build();
    auto tg = MGR::TrieGraphBuilder(std::move(g)).build();
    auto pos = trie2graph(tg, MGR::Kmer::from_str("acgt"));
    assert(pos.size() == 2);
    assert(pos[0] == 4);
    assert(pos[1] == 8);

    pos = trie2graph(tg, MGR::Kmer::from_str("cgta"));
    assert(pos.size() == 2);
    assert(pos[0] == 5);
    assert(pos[1] == 9);

    pos = trie2graph(tg, MGR::Kmer::from_str("gtac"));
    assert(pos.size() == 2);
    assert(pos[0] == 6);
    assert(pos[1] == tg.letter_loc.num_locations);
}

static void test_small_nonlinear_graph() {
    auto g = MGR::Graph::Builder()
        .add_node(MGR::Str("a"), "s1")
        .add_node(MGR::Str("cg"), "s2")
        .add_node(MGR::Str("t"), "s3")
        .add_node(MGR::Str("ac"), "s4")
        .add_edge("s1", "s2")
        .add_edge("s1", "s3")
        .add_edge("s2", "s4")
        .add_edge("s3", "s4")
        .build();

    /****************
     *     12       *
     *     cg       *
     *  a /  \ ac   *
     *  0 \  / 45 6 *
     *     t        *
     *     3        *
     ***************/
    auto tg = MGR::TrieGraphBuilder(std::move(g)).build();

    {
        auto pos = trie2graph(tg, MGR::Kmer::from_str("acga"));
        assert(pos.size() == 1);
        assert(pos[0] == 5);
    }

    {
        auto pos = trie2graph(tg, MGR::Kmer::from_str("atac"));
        assert(pos.size() == 1);
        assert(pos[0] == 6);
    }

    {
        auto pos = trie2graph(tg, MGR::Kmer::from_str("cgac"));
        assert(pos.size() == 1);
        assert(pos[0] == 6);
    }

    // std::cerr << tg.trie_data.active_trie.size() << std::endl;
    // for (const auto &kmer : tg.trie_data.active_trie) {
    //     std::cerr << "at " << kmer << std::endl;;
    // }
    assert(tg.trie_data.trie_inner_contains(MGR::Kmer::from_str("acg")));
    assert(tg.trie_data.trie_inner_contains(MGR::Kmer::from_str("cga")));
    assert(tg.trie_data.trie_inner_contains(MGR::Kmer::from_str("ata")));
    // assert(tg.trie_data.trie_inner_contains(DnaKmer::from_str("tac")));
    assert(tg.trie_data.trie_inner_contains(MGR::Kmer::from_str("cga")));
    // assert(tg.trie_data.trie_inner_contains(DnaKmer::from_str("gac")));
    // assert(tg.trie_data.active_trie.size() == 6);
}

int main() {
    test_tiny_linear_graph();
    test_small_nonlinear_graph();

    return 0;
}
