#include "util.h"
#include "rgfa_graph.h"
#include "dna_letter.h"
#include "letter_location_data.h"
#include "trie_data.h"
#include "dna_str.h"
#include "kmer.h"
#include "triegraph_builder.h"
#include "triegraph.h"

#include <assert.h>
#include <iostream>

using Graph = RgfaGraph<DnaStr, u32>;
using NPos = NodePos<u32>;
using LLData = LetterLocData<NPos, Graph>;
constexpr u64 on_mask = u64(1) << 63;
using DnaKmer = Kmer<DnaLetter, u64, 4, on_mask>;
using TData = TrieData<DnaKmer, u32>;
using LetterLoc = TData::LetterLoc;
using TrieG = Triegraph<Graph, LLData, TData>;
using TB = TriegraphBuilder<Graph, LLData, TData, TrieG>;

static std::vector<LetterLoc> trie2graph(const TrieG &tg, DnaKmer kmer) {
    auto pos_it = tg.trie_data.trie2graph.equal_range(kmer);
    std::vector<LetterLoc> pos;
    std::transform(pos_it.first, pos_it.second, std::back_inserter(pos),
            [](const std::pair<DnaKmer, LetterLoc> &a) { return a.second; });
    std::sort(pos.begin(), pos.end());
    return pos;
}

static void test_tiny_linear_graph() {
    Graph g;
    g.nodes.emplace_back(DnaStr("acgtacgtac"), "s1");
    g.edge_start = { Graph::INV_SIZE };
    g.redge_start = { Graph::INV_SIZE };

    auto builder = TB(std::move(g));
    auto tg = builder.build();

    auto pos = trie2graph(tg, DnaKmer::from_str("acgt"));
    assert(pos.size() == 2);
    assert(pos[0] == 4);
    assert(pos[1] == 8);

    pos = trie2graph(tg, DnaKmer::from_str("cgta"));
    assert(pos.size() == 2);
    assert(pos[0] == 5);
    assert(pos[1] == 9);

    pos = trie2graph(tg, DnaKmer::from_str("gtac"));
    assert(pos.size() == 2);
    assert(pos[0] == 6);
    assert(pos[1] == tg.letter_loc.num_locations);
}

static void test_small_nonlinear_graph() {
    Graph g;
    g.nodes.emplace_back(DnaStr("a"), "s1");
    g.nodes.emplace_back(DnaStr("cg"), "s2");
    g.nodes.emplace_back(DnaStr("t"), "s3");
    g.nodes.emplace_back(DnaStr("ac"), "s4");
    g.edges.emplace_back(1, Graph::INV_SIZE);
    g.edges.emplace_back(0, Graph::INV_SIZE);
    g.edges.emplace_back(2, 0);
    g.edges.emplace_back(0, Graph::INV_SIZE);
    g.edges.emplace_back(3, Graph::INV_SIZE);
    g.edges.emplace_back(1, Graph::INV_SIZE);
    g.edges.emplace_back(3, Graph::INV_SIZE);
    g.edges.emplace_back(2, 5);
    g.edge_start = {2, 4, 6, Graph::INV_SIZE};
    g.redge_start = {Graph::INV_SIZE, 1, 3, 7};

    // std::cerr << g;
    auto builder = TB(std::move(g));
    auto tg = builder.build();

    {
        auto pos = trie2graph(tg, DnaKmer::from_str("acga"));
        assert(pos.size() == 1);
        assert(pos[0] == 5);
    }

    {
        auto pos = trie2graph(tg, DnaKmer::from_str("atac"));
        assert(pos.size() == 1);
        assert(pos[0] == 6);
    }

    {
        auto pos = trie2graph(tg, DnaKmer::from_str("cgac"));
        assert(pos.size() == 1);
        assert(pos[0] == 6);
    }

    // std::cerr << tg.trie_data.active_trie.size() << std::endl;
    // for (const auto &kmer : tg.trie_data.active_trie) {
    //     std::cerr << "at " << kmer << std::endl;;
    // }
    assert(tg.trie_data.active_trie.contains(DnaKmer::from_str("acg")));
    assert(tg.trie_data.active_trie.contains(DnaKmer::from_str("cga")));
    assert(tg.trie_data.active_trie.contains(DnaKmer::from_str("ata")));
    // assert(tg.trie_data.active_trie.contains(DnaKmer::from_str("tac")));
    assert(tg.trie_data.active_trie.contains(DnaKmer::from_str("cga")));
    // assert(tg.trie_data.active_trie.contains(DnaKmer::from_str("gac")));
    // assert(tg.trie_data.active_trie.size() == 6);
}

int main() {
    test_tiny_linear_graph();
    test_small_nonlinear_graph();

    return 0;
}
