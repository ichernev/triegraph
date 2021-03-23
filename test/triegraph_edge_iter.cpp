// #include "util/util.h"
// #include "alphabet/dna_letter.h"
#include "dna_config.h"
#include "manager.h"

#include <assert.h>
#include <iostream>
#include <ranges>

using namespace triegraph;

namespace {

using TG = Manager<dna::DnaConfig<15>>;

using Kmer = TG::Kmer;
using EditEdge = TG::EditEdge;
using NodePos = TG::NodePos;
using DnaLetters = TG::Letters;
using EditEdgeIter = TG::TrieGraph::edit_edge_iterator;
using EditEdgeIterView = TG::TrieGraph::edit_edge_iter_view;

void func(std::forward_iterator auto it) {}

static void test_graph_fwd() {
    EditEdgeIterView h = EditEdgeIter::make_graph_fwd(DnaLetters::C, NodePos { 4, 2 });
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

    assert(std::ranges::equal(h, expected));
}

static void test_graph_split() {
    auto g = TG::Graph::Builder()
        .add_node(TG::Str("a"), "s1")
        .add_node(TG::Str("c"), "s2")
        .add_node(TG::Str("g"), "s3")
        .add_edge("s1", "s3")
        .add_edge("s1", "s2")
        .build();

    EditEdgeIterView h = EditEdgeIter::make_graph_split(
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
    // std::ranges::copy(h, std::ostream_iterator<EditEdge>(std::cerr, "\n"));
    assert(std::ranges::equal(h, expected));

    // this simulates end-of-graph
    h = EditEdgeIter::make_graph_split(
                DnaLetters::A,
                NodePos { 0, 0 },
                g.forward_from(2));
    expected = {
        EditEdge { NodePos { 0, 0 }, EditEdge::INS,   DnaLetters::A },
        EditEdge { NodePos { 0, 0 }, EditEdge::INS,   DnaLetters::C },
        EditEdge { NodePos { 0, 0 }, EditEdge::INS,   DnaLetters::G },
        EditEdge { NodePos { 0, 0 }, EditEdge::INS,   DnaLetters::T },
    };
    // std::ranges::copy(h, std::ostream_iterator<EditEdge>(std::cerr, "\n"));
    assert(std::ranges::equal(h, expected));
}

static void test_trie_inner() {
    EditEdgeIterView h = EditEdgeIter::make_trie_inner(
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
    // std::ranges::copy(h, std::ostream_iterator<EditEdge>(std::cerr, "\n"));
    assert(std::ranges::equal(h, expected));
}

static void test_trie_to_graph() {
    using TG2 = Manager<dna::DnaConfig<2>>;

    using Kmer = TG2::Kmer;
    using EditEdge = TG2::EditEdge;
    using NodePos = TG2::NodePos;
    using DnaLetters = dna::DnaLetters;
    using DnaStr = TG2::Str;

    auto g = TG2::Graph::Builder()
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

    // build a trie, so we can test trie-to-graph iter
    auto tg = TG2::TrieGraphBuilder(std::move(g)).build();

    typename TG2::TrieGraph::edit_edge_iter_view h =
        TG2::TrieGraph::edit_edge_iterator::make_trie_to_graph(
                Kmer::from_str("ac"), tg);

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

    std::ranges::copy(h, std::ostream_iterator<EditEdge>(std::cerr, "\n"));
    assert(std::ranges::equal(h, expected));
}

} /* unnamed namespace */

int main() {
    test_graph_fwd();
    test_graph_split();
    test_trie_inner();
    // test_trie_to_graph();

    return 0;
}
