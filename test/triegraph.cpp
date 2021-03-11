#include "dna_config.h"
#include "manager.h"

#include <assert.h>
#include <iostream>

using namespace triegraph;

namespace {

using M4 = Manager<dna::DnaConfig<4>>;
using Str = M4::Str;
using EditEdge = M4::EditEdge;
using NodePos = M4::NodePos;
using DnaLetters = M4::Letters;
using Kmer = M4::Kmer;

M4::Graph build_graph() {
    return M4::Graph::Builder()
        .add_node(Str("ac"), "s1")
        .add_node(Str("gg"), "s2")
        .add_node(Str("acg"), "s3")
        .add_node(Str("ac"), "s4")
        .add_edge("s1", "s2")
        .add_edge("s1", "s3")
        .add_edge("s2", "s4")
        .add_edge("s3", "s4")
        .build();
}

static void test_next_edit_edges_fwd() {
    auto tg = M4::triegraph_from_graph(
            build_graph(),
            M4::Settings()
                .add_reverse_complement(false)
                .trie_depth(4));

    auto h = tg.next_edit_edges(M4::Handle(2, 1));
    auto expected = std::vector<M4::EditEdge> {
        EditEdge { NodePos { 2, 2 }, EditEdge::SUB,   DnaLetters::A },
        EditEdge { NodePos { 2, 2 }, EditEdge::MATCH, DnaLetters::C },
        EditEdge { NodePos { 2, 2 }, EditEdge::SUB,   DnaLetters::G },
        EditEdge { NodePos { 2, 2 }, EditEdge::SUB,   DnaLetters::T },

        EditEdge { NodePos { 2, 1 }, EditEdge::INS,   DnaLetters::A },
        EditEdge { NodePos { 2, 1 }, EditEdge::INS,   DnaLetters::C },
        EditEdge { NodePos { 2, 1 }, EditEdge::INS,   DnaLetters::G },
        EditEdge { NodePos { 2, 1 }, EditEdge::INS,   DnaLetters::T },

        EditEdge { NodePos { 2, 2 }, EditEdge::DEL,   DnaLetters::EPS },
    };

    assert(std::equal(h.begin(), h.end(), expected.begin()));
}

static void test_next_edit_edges_split() {
    auto tg = M4::triegraph_from_graph(
            build_graph(),
            M4::Settings()
                .add_reverse_complement(false)
                .trie_depth(4));

    auto h = tg.next_edit_edges(M4::Handle(0, 1));
    auto expected = std::vector<M4::EditEdge> {
        EditEdge { NodePos { 2, 0 }, EditEdge::SUB,   DnaLetters::A },
        EditEdge { NodePos { 2, 0 }, EditEdge::MATCH, DnaLetters::C },
        EditEdge { NodePos { 2, 0 }, EditEdge::SUB,   DnaLetters::G },
        EditEdge { NodePos { 2, 0 }, EditEdge::SUB,   DnaLetters::T },
        EditEdge { NodePos { 2, 0 }, EditEdge::DEL,   DnaLetters::EPS },

        EditEdge { NodePos { 1, 0 }, EditEdge::SUB,   DnaLetters::A },
        EditEdge { NodePos { 1, 0 }, EditEdge::MATCH, DnaLetters::C },
        EditEdge { NodePos { 1, 0 }, EditEdge::SUB,   DnaLetters::G },
        EditEdge { NodePos { 1, 0 }, EditEdge::SUB,   DnaLetters::T },
        EditEdge { NodePos { 1, 0 }, EditEdge::DEL,   DnaLetters::EPS },

        EditEdge { NodePos { 0, 1 }, EditEdge::INS,   DnaLetters::A },
        EditEdge { NodePos { 0, 1 }, EditEdge::INS,   DnaLetters::C },
        EditEdge { NodePos { 0, 1 }, EditEdge::INS,   DnaLetters::G },
        EditEdge { NodePos { 0, 1 }, EditEdge::INS,   DnaLetters::T },
    };

    assert(std::equal(h.begin(), h.end(), expected.begin()));
}

static void test_next_edit_edges_trie_inner() {
    auto tg = M4::triegraph_from_graph(
            build_graph(),
            M4::Settings()
                .add_reverse_complement(false)
                .trie_depth(4));

    auto h = tg.next_edit_edges(M4::Handle(Kmer::from_str("cg")));

    auto expected = std::vector<M4::EditEdge> {
        EditEdge { Kmer::from_str("cga"), EditEdge::MATCH, DnaLetters::A },
        EditEdge { Kmer::from_str("cga"), EditEdge::SUB, DnaLetters::C },
        EditEdge { Kmer::from_str("cga"), EditEdge::SUB, DnaLetters::G },
        EditEdge { Kmer::from_str("cga"), EditEdge::SUB, DnaLetters::T },
        EditEdge { Kmer::from_str("cga"), EditEdge::DEL, DnaLetters::EPS },

        EditEdge { Kmer::from_str("cgg"), EditEdge::SUB, DnaLetters::A },
        EditEdge { Kmer::from_str("cgg"), EditEdge::SUB, DnaLetters::C },
        EditEdge { Kmer::from_str("cgg"), EditEdge::MATCH, DnaLetters::G },
        EditEdge { Kmer::from_str("cgg"), EditEdge::SUB, DnaLetters::T },
        EditEdge { Kmer::from_str("cgg"), EditEdge::DEL, DnaLetters::EPS },

        EditEdge { Kmer::from_str("cg"), EditEdge::INS, DnaLetters::A },
        EditEdge { Kmer::from_str("cg"), EditEdge::INS, DnaLetters::C },
        EditEdge { Kmer::from_str("cg"), EditEdge::INS, DnaLetters::G },
        EditEdge { Kmer::from_str("cg"), EditEdge::INS, DnaLetters::T },
    };

    // std::copy(h.begin(), h.end(), std::ostream_iterator<EditEdge>(std::cerr, "\n"));
    assert(std::equal(h.begin(), h.end(), expected.begin()));
}

static void test_next_edit_edges_trie_edge() {
    auto tg = M4::triegraph_from_graph(
            build_graph(),
            M4::Settings()
                .add_reverse_complement(false)
                .trie_depth(4));

    auto h = tg.next_edit_edges(M4::Handle(Kmer::from_str("cga")));

    auto expected = std::vector<M4::EditEdge> {
        EditEdge { Kmer::from_str("cgac"), EditEdge::SUB, DnaLetters::A },
        EditEdge { Kmer::from_str("cgac"), EditEdge::MATCH, DnaLetters::C },
        EditEdge { Kmer::from_str("cgac"), EditEdge::SUB, DnaLetters::G },
        EditEdge { Kmer::from_str("cgac"), EditEdge::SUB, DnaLetters::T },
        EditEdge { Kmer::from_str("cgac"), EditEdge::DEL, DnaLetters::EPS },

        EditEdge { Kmer::from_str("cga"), EditEdge::INS, DnaLetters::A },
        EditEdge { Kmer::from_str("cga"), EditEdge::INS, DnaLetters::C },
        EditEdge { Kmer::from_str("cga"), EditEdge::INS, DnaLetters::G },
        EditEdge { Kmer::from_str("cga"), EditEdge::INS, DnaLetters::T },
    };

    // std::copy(h.begin(), h.end(), std::ostream_iterator<EditEdge>(std::cerr, "\n"));
    assert(std::equal(h.begin(), h.end(), expected.begin()));
}


static void test_next_edit_edges_trie_to_graph() {
    auto tg = M4::triegraph_from_graph(
            build_graph(),
            M4::Settings()
                .add_reverse_complement(false)
                .trie_depth(4));

    auto h = tg.next_edit_edges(M4::Handle(Kmer::from_str("acgg")));

    // auto er = tg.data.trie_data.trie2graph.equal_range(Kmer::from_str("acgg"));
    // for (auto it = er.first; it != er.second; ++it) {
    //     std::cerr << it->second << std::endl;
    // }

    auto expected = std::vector<M4::EditEdge> {
        EditEdge { NodePos { 3, 1 }, EditEdge::MATCH, DnaLetters::A },
        EditEdge { NodePos { 3, 1 }, EditEdge::SUB,   DnaLetters::C },
        EditEdge { NodePos { 3, 1 }, EditEdge::SUB,   DnaLetters::G },
        EditEdge { NodePos { 3, 1 }, EditEdge::SUB,   DnaLetters::T },

        EditEdge { NodePos { 3, 0 }, EditEdge::INS,   DnaLetters::A },
        EditEdge { NodePos { 3, 0 }, EditEdge::INS,   DnaLetters::C },
        EditEdge { NodePos { 3, 0 }, EditEdge::INS,   DnaLetters::G },
        EditEdge { NodePos { 3, 0 }, EditEdge::INS,   DnaLetters::T },

        EditEdge { NodePos { 3, 1 }, EditEdge::DEL,   DnaLetters::EPS },
    };

    // std::copy(h.begin(), h.end(), std::ostream_iterator<EditEdge>(std::cerr, "\n"));
    assert(std::equal(h.begin(), h.end(), expected.begin()));
}

} /* unnamed namespace */

int main() {
    test_next_edit_edges_fwd();
    test_next_edit_edges_split();
    test_next_edit_edges_trie_inner();
    test_next_edit_edges_trie_edge();
    test_next_edit_edges_trie_to_graph();

    return 0;
}
