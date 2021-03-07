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
    assert(pos.size() == 1);
    assert(pos[0] == 6);
}

int main() {
    test_tiny_linear_graph();

    return 0;
}
