#include "util.h"
#include "edge.h"
#include "kmer.h"
#include "letter.h"
#include "dna_letter.h"
#include "dna_str.h"
#include "rgfa_graph.h"
#include "trie_data.h"
#include "triegraph_data.h"
#include "triegraph_builder.h"
#include "letter_location_data.h"

#include <assert.h>
#include <iostream>

namespace {
constexpr u64 on_mask = u64(1) << 63;
using Kmer = Kmer<DnaLetter, u64, 31, on_mask>;
using NodePos = NodePos<u32>;
using Handle = Handle<Kmer, NodePos, u32>;
using EditEdge = EditEdge<Handle, DnaLetter>;
using Graph = RgfaGraph<DnaStr, u32>;
using LetterLocData = LetterLocData<NodePos, Graph>;
using TrieData = TrieData<Kmer, u32>;
using TrieGraphData = TrieGraphData<Graph, LetterLocData, TrieData>;
using TrieGraphBuilder = TriegraphBuilder<TrieGraphData>;

using ImplHolder = EditEdgeImplHolder<Handle, DnaLetter, TrieGraphData>;
using Helper = EditEdgeIterHelper<ImplHolder>;

static_assert(sizeof(Kmer) == sizeof(NodePos));

template<std::forward_iterator IT>
void func(IT it) {}

static void test_graph_fwd() {
    // using EdgeIterImplGraphFwd_X = EdgeIterImplGraphFwd<Handle, DnaLetter>;
    // auto it = EdgeIterGraphFwd_X { DnaLetters::C, NodePos { 4, 2 } };
    Helper h(ImplHolder::make_graph_fwd(DnaLetters::C, NodePos { 4, 2 }));
    std::vector<EditEdge> expected = {
        EditEdge { NodePos { 4, 3 }, EditEdge::SUB,   DnaLetters::A },
        EditEdge { NodePos { 4, 3 }, EditEdge::MATCH, DnaLetters::C },
        EditEdge { NodePos { 4, 3 }, EditEdge::SUB,   DnaLetters::G },
        EditEdge { NodePos { 4, 3 }, EditEdge::SUB,   DnaLetters::T },

        EditEdge { NodePos { 4, 2 }, EditEdge::INS,   DnaLetters::A },
        EditEdge { NodePos { 4, 2 }, EditEdge::INS,   DnaLetters::C },
        EditEdge { NodePos { 4, 2 }, EditEdge::INS,   DnaLetters::G },
        EditEdge { NodePos { 4, 2 }, EditEdge::INS,   DnaLetters::T },

        EditEdge { NodePos { 4, 3 }, EditEdge::DEL,   DnaLetters::EPS },
    };

    assert(std::equal(h.begin(), h.end(), expected.begin()));
    // std::vector<EditEdge> actual;
    // std::copy(h.begin(), h.end(), std::back_inserter(actual));
    // assert(expected == actual);
}

static void test_graph_split() {
    auto g = Graph::Builder()
        .add_node(DnaStr("a"), "s1")
        .add_node(DnaStr("c"), "s2")
        .add_node(DnaStr("g"), "s3")
        .add_edge("s1", "s3")
        .add_edge("s1", "s2")
        .build();

    Helper h(ImplHolder::make_graph_split(
                DnaLetters::A,
                NodePos { 0, 0 },
                g.forward_from(0).begin()));
    std::vector<EditEdge> expected = {
        EditEdge { NodePos { 1, 0 }, EditEdge::MATCH, DnaLetters::A },
        EditEdge { NodePos { 1, 0 }, EditEdge::SUB,   DnaLetters::C },
        EditEdge { NodePos { 1, 0 }, EditEdge::SUB,   DnaLetters::G },
        EditEdge { NodePos { 1, 0 }, EditEdge::SUB,   DnaLetters::T },
        EditEdge { NodePos { 1, 0 }, EditEdge::DEL,   DnaLetters::EPS },

        EditEdge { NodePos { 2, 0 }, EditEdge::MATCH, DnaLetters::A },
        EditEdge { NodePos { 2, 0 }, EditEdge::SUB,   DnaLetters::C },
        EditEdge { NodePos { 2, 0 }, EditEdge::SUB,   DnaLetters::G },
        EditEdge { NodePos { 2, 0 }, EditEdge::SUB,   DnaLetters::T },
        EditEdge { NodePos { 2, 0 }, EditEdge::DEL,   DnaLetters::EPS },

        EditEdge { NodePos { 0, 0 }, EditEdge::INS,   DnaLetters::A },
        EditEdge { NodePos { 0, 0 }, EditEdge::INS,   DnaLetters::C },
        EditEdge { NodePos { 0, 0 }, EditEdge::INS,   DnaLetters::G },
        EditEdge { NodePos { 0, 0 }, EditEdge::INS,   DnaLetters::T },
    };
    // std::copy(h.begin(), h.end(), std::ostream_iterator<EditEdge>(std::cerr, "\n"));
    assert(std::equal(h.begin(), h.end(), expected.begin()));

    // this simulates end-of-graph
    Helper h2(ImplHolder::make_graph_split(
                DnaLetters::A,
                NodePos { 0, 0 },
                g.forward_from(2).begin()));
    std::vector<EditEdge> expected2 = {
        EditEdge { NodePos { 0, 0 }, EditEdge::INS,   DnaLetters::A },
        EditEdge { NodePos { 0, 0 }, EditEdge::INS,   DnaLetters::C },
        EditEdge { NodePos { 0, 0 }, EditEdge::INS,   DnaLetters::G },
        EditEdge { NodePos { 0, 0 }, EditEdge::INS,   DnaLetters::T },
    };
    // std::copy(h.begin(), h.end(), std::ostream_iterator<EditEdge>(std::cerr, "\n"));
    assert(std::equal(h2.begin(), h2.end(), expected2.begin()));
}

static void test_trie_inner() {
    Helper h(ImplHolder::make_trie_inner(
                Kmer::from_str("acgt"),
                (1 << 1) | (1 << 2)));

    std::vector<EditEdge> expected = {
        EditEdge { Kmer::from_str("acgtc"), EditEdge::SUB,   DnaLetters::A },
        EditEdge { Kmer::from_str("acgtc"), EditEdge::MATCH, DnaLetters::C },
        EditEdge { Kmer::from_str("acgtc"), EditEdge::SUB,   DnaLetters::G },
        EditEdge { Kmer::from_str("acgtc"), EditEdge::SUB,   DnaLetters::T },
        EditEdge { Kmer::from_str("acgtc"), EditEdge::DEL,   DnaLetters::EPS },

        EditEdge { Kmer::from_str("acgtg"), EditEdge::SUB,   DnaLetters::A },
        EditEdge { Kmer::from_str("acgtg"), EditEdge::SUB,   DnaLetters::C },
        EditEdge { Kmer::from_str("acgtg"), EditEdge::MATCH, DnaLetters::G },
        EditEdge { Kmer::from_str("acgtg"), EditEdge::SUB,   DnaLetters::T },
        EditEdge { Kmer::from_str("acgtg"), EditEdge::DEL,   DnaLetters::EPS },

        EditEdge { Kmer::from_str("acgt"), EditEdge::INS,   DnaLetters::A },
        EditEdge { Kmer::from_str("acgt"), EditEdge::INS,   DnaLetters::C },
        EditEdge { Kmer::from_str("acgt"), EditEdge::INS,   DnaLetters::G },
        EditEdge { Kmer::from_str("acgt"), EditEdge::INS,   DnaLetters::T },
    };
    // std::copy(h.begin(), h.end(), std::ostream_iterator<EditEdge>(std::cerr, "\n"));
    assert(std::equal(h.begin(), h.end(), expected.begin()));
}

static void test_trie_to_graph() {
    using Kmer = ::Kmer<DnaLetter, u64, 2, on_mask>;
    using Handle = ::Handle<Kmer, NodePos, u32>;
    using EditEdge = ::EditEdge<Handle, DnaLetter>;
    using TrieData = ::TrieData<Kmer, u32>;
    using TrieGraphData = ::TrieGraphData<Graph, LetterLocData, TrieData>;
    using TrieGraphBuilder = ::TriegraphBuilder<TrieGraphData>;
    using ImplHolder = ::EditEdgeImplHolder<Handle, DnaLetter, TrieGraphData>;
    using Helper = ::EditEdgeIterHelper<ImplHolder>;

    auto g = Graph::Builder()
        .add_node(DnaStr("ac"), "s1")
        .add_node(DnaStr("gg"), "s2")
        .add_node(DnaStr("acg"), "s3")
        .add_node(DnaStr("ac"), "s4")
        .add_edge("s1", "s2")
        .add_edge("s1", "s3")
        .add_edge("s2", "s4")
        .add_edge("s3", "s4")
        .build();

     /******************
      *      [1]       *
      *  [0] GG   [3]  *
      *  AC /23 \ AC   *
      *  01 \[2]/ 78 9 *
      *      ACG       *
      *      456       *
      ******************/

    auto tg = TrieGraphBuilder(std::move(g)).build();

    Helper h(ImplHolder::make_trie_to_graph(Kmer::from_str("ac"), tg));

    // TODO: Should we emit INS nodes, when there are already INS nodes in the
    // graph.
    std::vector<EditEdge> expected = {
        EditEdge { Kmer::from_str("ac"), EditEdge::INS,   DnaLetters::A },
        EditEdge { Kmer::from_str("ac"), EditEdge::INS,   DnaLetters::C },
        EditEdge { Kmer::from_str("ac"), EditEdge::INS,   DnaLetters::G },
        EditEdge { Kmer::from_str("ac"), EditEdge::INS,   DnaLetters::T },

        EditEdge { NodePos(3, 0), EditEdge::SUB,   DnaLetters::A },
        EditEdge { NodePos(3, 0), EditEdge::SUB,   DnaLetters::C },
        EditEdge { NodePos(3, 0), EditEdge::MATCH, DnaLetters::G },
        EditEdge { NodePos(3, 0), EditEdge::SUB,   DnaLetters::T },
        EditEdge { NodePos(3, 0), EditEdge::DEL,   DnaLetters::EPS },
        EditEdge { NodePos(2, 2), EditEdge::INS,   DnaLetters::A },
        EditEdge { NodePos(2, 2), EditEdge::INS,   DnaLetters::C },
        EditEdge { NodePos(2, 2), EditEdge::INS,   DnaLetters::G },
        EditEdge { NodePos(2, 2), EditEdge::INS,   DnaLetters::T },

        EditEdge { NodePos(2, 1), EditEdge::MATCH, DnaLetters::A },
        EditEdge { NodePos(2, 1), EditEdge::SUB,   DnaLetters::C },
        EditEdge { NodePos(2, 1), EditEdge::SUB,   DnaLetters::G },
        EditEdge { NodePos(2, 1), EditEdge::SUB,   DnaLetters::T },
        EditEdge { NodePos(2, 0), EditEdge::INS,   DnaLetters::A },
        EditEdge { NodePos(2, 0), EditEdge::INS,   DnaLetters::C },
        EditEdge { NodePos(2, 0), EditEdge::INS,   DnaLetters::G },
        EditEdge { NodePos(2, 0), EditEdge::INS,   DnaLetters::T },
        EditEdge { NodePos(2, 1), EditEdge::DEL,   DnaLetters::EPS },

        EditEdge { NodePos(1, 1), EditEdge::SUB,   DnaLetters::A },
        EditEdge { NodePos(1, 1), EditEdge::SUB,   DnaLetters::C },
        EditEdge { NodePos(1, 1), EditEdge::MATCH, DnaLetters::G },
        EditEdge { NodePos(1, 1), EditEdge::SUB,   DnaLetters::T },
        EditEdge { NodePos(1, 0), EditEdge::INS,   DnaLetters::A },
        EditEdge { NodePos(1, 0), EditEdge::INS,   DnaLetters::C },
        EditEdge { NodePos(1, 0), EditEdge::INS,   DnaLetters::G },
        EditEdge { NodePos(1, 0), EditEdge::INS,   DnaLetters::T },
        EditEdge { NodePos(1, 1), EditEdge::DEL,   DnaLetters::EPS },
    };

    // std::copy(h.begin(), h.end(), std::ostream_iterator<EditEdge>(std::cerr, "\n"));
    assert(std::equal(h.begin(), h.end(), expected.begin()));
}


} /* unnamed namespace */

int main() {
    test_graph_fwd();
    test_graph_split();
    test_trie_inner();
    test_trie_to_graph();

    return 0;
}
