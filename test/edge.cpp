#include "util.h"
#include "edge.h"
#include "kmer.h"
#include "letter.h"
#include "dna_letter.h"
#include "dna_str.h"
#include "rgfa_graph.h"
#include "letter_location_data.h"

#include <assert.h>
#include <iostream>

namespace {
constexpr u64 on_mask = u64(1) << 63;
using DnaKmer31 = Kmer<DnaLetter, u64, 31, on_mask>;
using NodePos = NodePos<u32>;
using Handle = Handle<DnaKmer31, NodePos, u32>;
using EditEdge = EditEdge<Handle, DnaLetter>;
using Graph = RgfaGraph<DnaStr, u32>;

using ImplHolder = EditEdgeImplHolder<Handle, DnaLetter, Graph>;
using Helper = EditEdgeIterHelper<ImplHolder>;

static_assert(sizeof(DnaKmer31) == sizeof(NodePos));

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
}

} /* unnamed namespace */

int main() {
    test_graph_fwd();
    test_graph_split();

    return 0;
}
