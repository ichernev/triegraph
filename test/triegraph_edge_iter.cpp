#include "util/util.h"
#include "alphabet/dna_letter.h"
#include "manager.h"
#include "triegraph/triegraph_edge_iter.h"

#include <assert.h>
#include <iostream>

using namespace triegraph;

namespace {

struct Cfg {
    using Letter = dna::DnaLetter;
    using StrHolder = u32;
    using NodeLoc = u32;
    using NodeLen = u32;
    using EdgeLoc = u32;
    using LetterLoc = u32;
    using KmerHolder = u32;
    static constexpr int LetterLocIdxShift = -1;
    static constexpr u64 KmerLen = 15;
    static constexpr KmerHolder on_mask = KmerHolder(1) << 31;
};

using MGR = Manager<Cfg>;

using Kmer = MGR::Kmer;
using EditEdge = MGR::EditEdge;
using NodePos = MGR::NodePos;
using DnaLetters = dna::DnaLetters;

template<std::forward_iterator IT>
void func(IT it) {}

static void test_graph_fwd() {
    // using EdgeIterImplGraphFwd_X = EdgeIterImplGraphFwd<Handle, DnaLetter>;
    // auto it = EdgeIterGraphFwd_X { DnaLetters::C, NodePos { 4, 2 } };
    // Helper h(ImplHolder::make_graph_fwd(DnaLetters::C, NodePos { 4, 2 }));
    MGR::EditEdgeIterHelper h = MGR::EditEdgeIter::make_graph_fwd(DnaLetters::C, NodePos { 4, 2 });
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
    auto g = MGR::Graph::Builder()
        .add_node(MGR::Str("a"), "s1")
        .add_node(MGR::Str("c"), "s2")
        .add_node(MGR::Str("g"), "s3")
        .add_edge("s1", "s3")
        .add_edge("s1", "s2")
        .build();

    MGR::EditEdgeIterHelper h = MGR::EditEdgeIter::make_graph_split(
            DnaLetters::A,
            NodePos { 0, 0 },
            g.forward_from(0));
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
    h = MGR::EditEdgeIter::make_graph_split(
                DnaLetters::A,
                NodePos { 0, 0 },
                g.forward_from(2));
    expected = {
        EditEdge { NodePos { 0, 0 }, EditEdge::INS,   DnaLetters::A },
        EditEdge { NodePos { 0, 0 }, EditEdge::INS,   DnaLetters::C },
        EditEdge { NodePos { 0, 0 }, EditEdge::INS,   DnaLetters::G },
        EditEdge { NodePos { 0, 0 }, EditEdge::INS,   DnaLetters::T },
    };
    // std::copy(h.begin(), h.end(), std::ostream_iterator<EditEdge>(std::cerr, "\n"));
    assert(std::equal(h.begin(), h.end(), expected.begin()));
}

static void test_trie_inner() {
    MGR::EditEdgeIterHelper h = MGR::EditEdgeIter::make_trie_inner(
            Kmer::from_str("acgt"),
            (1 << 1) | (1 << 2));

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

struct Cfg2 {
    using Letter = dna::DnaLetter;
    using StrHolder = u32;
    using NodeLoc = u32;
    using NodeLen = u32;
    using EdgeLoc = u32;
    using LetterLoc = u32;
    using KmerHolder = u32;
    static constexpr int LetterLocIdxShift = -1;
    static constexpr u64 KmerLen = 2;
    static constexpr KmerHolder on_mask = KmerHolder(1) << 31;
};

static void test_trie_to_graph() {

    using MGR2 = Manager<Cfg2>;

    using Kmer = MGR2::Kmer;
    using EditEdge = MGR2::EditEdge;
    using NodePos = MGR2::NodePos;
    using DnaLetters = dna::DnaLetters;
    using DnaStr = MGR2::Str;

    auto g = MGR2::Graph::Builder()
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

    auto tg = MGR2::TrieGraphBuilder(std::move(g)).build();

    MGR2::EditEdgeIterHelper h = MGR2::EditEdgeIter::make_trie_to_graph(
            Kmer::from_str("ac"), tg);
    // Helper h(ImplHolder::make_trie_to_graph();

    std::vector<EditEdge> expected = {

        EditEdge { NodePos(1, 1), EditEdge::SUB,   DnaLetters::A },
        EditEdge { NodePos(1, 1), EditEdge::SUB,   DnaLetters::C },
        EditEdge { NodePos(1, 1), EditEdge::MATCH, DnaLetters::G },
        EditEdge { NodePos(1, 1), EditEdge::SUB,   DnaLetters::T },
        EditEdge { NodePos(1, 0), EditEdge::INS,   DnaLetters::A },
        EditEdge { NodePos(1, 0), EditEdge::INS,   DnaLetters::C },
        EditEdge { NodePos(1, 0), EditEdge::INS,   DnaLetters::G },
        EditEdge { NodePos(1, 0), EditEdge::INS,   DnaLetters::T },
        EditEdge { NodePos(1, 1), EditEdge::DEL,   DnaLetters::EPS },

        EditEdge { NodePos(2, 1), EditEdge::MATCH, DnaLetters::A },
        EditEdge { NodePos(2, 1), EditEdge::SUB,   DnaLetters::C },
        EditEdge { NodePos(2, 1), EditEdge::SUB,   DnaLetters::G },
        EditEdge { NodePos(2, 1), EditEdge::SUB,   DnaLetters::T },
        EditEdge { NodePos(2, 0), EditEdge::INS,   DnaLetters::A },
        EditEdge { NodePos(2, 0), EditEdge::INS,   DnaLetters::C },
        EditEdge { NodePos(2, 0), EditEdge::INS,   DnaLetters::G },
        EditEdge { NodePos(2, 0), EditEdge::INS,   DnaLetters::T },
        EditEdge { NodePos(2, 1), EditEdge::DEL,   DnaLetters::EPS },

        EditEdge { NodePos(3, 0), EditEdge::SUB,   DnaLetters::A },
        EditEdge { NodePos(3, 0), EditEdge::SUB,   DnaLetters::C },
        EditEdge { NodePos(3, 0), EditEdge::MATCH, DnaLetters::G },
        EditEdge { NodePos(3, 0), EditEdge::SUB,   DnaLetters::T },
        EditEdge { NodePos(3, 0), EditEdge::DEL,   DnaLetters::EPS },

        EditEdge { NodePos(2, 2), EditEdge::INS,   DnaLetters::A },
        EditEdge { NodePos(2, 2), EditEdge::INS,   DnaLetters::C },
        EditEdge { NodePos(2, 2), EditEdge::INS,   DnaLetters::G },
        EditEdge { NodePos(2, 2), EditEdge::INS,   DnaLetters::T },

        EditEdge { NodePos(3, 2), EditEdge::INS,   DnaLetters::A },
        EditEdge { NodePos(3, 2), EditEdge::INS,   DnaLetters::C },
        EditEdge { NodePos(3, 2), EditEdge::INS,   DnaLetters::G },
        EditEdge { NodePos(3, 2), EditEdge::INS,   DnaLetters::T },
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
