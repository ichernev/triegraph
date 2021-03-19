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

     /******************
      *      [1]       *
      *  [0] GG   [3]  *
      *  AC /23 \ AC   *
      *  01 \[2]/ 78 9 *
      *      ACG       *
      *      456       *
      ******************/
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

template<typename IT>
static std::vector<typename IT::value_type> _collect_it(IT it) {
    std::vector<typename IT::value_type> res;
    while (it.has_more()) {
        res.emplace_back(*it);
        ++it;
    }
    return res;
}

static void test_prev_handles_linear() {
    auto tg = M4::triegraph_from_graph(
            build_graph(),
            M4::Settings()
                .add_reverse_complement(false)
                .trie_depth(4));

    auto h = tg.prev_graph_handles(M4::Handle(0, 1));
    // auto hh = h.begin();
    // std::cerr << hh.single.h << std::endl;
    // std::cerr << hh.single.has_more() << std::endl;
    // ++hh;
    // std::cerr << hh.single.h << std::endl;
    // std::cerr << hh.single.has_more() << std::endl;
    // for (const auto &x: h) {
    //     std::cerr << "HHH " << std::endl;
    // }
    auto expected = std::vector<M4::Handle> { { 0, 0 } };
    assert(std::equal(h.begin(), h.end(), expected.begin()));

    auto itv = _collect_it(tg.prev_graph_handles_it(M4::Handle(0, 1)));
    assert(std::equal(itv.begin(), itv.end(), expected.begin()));
}

static void test_prev_handles_split() {
    auto tg = M4::triegraph_from_graph(
            build_graph(),
            M4::Settings()
                .add_reverse_complement(false)
                .trie_depth(4));

    auto h = tg.prev_graph_handles(M4::Handle(3, 0));
    auto expected = std::vector<M4::Handle> { { 2, 2 }, { 1, 1 } };
    // std::copy(h.begin(), h.end(), std::ostream_iterator<M4::Handle>(std::cerr, "\n"));
    assert(std::equal(h.begin(), h.end(), expected.begin()));

    auto itv = _collect_it(tg.prev_graph_handles_it(M4::Handle(3, 0)));
    // std::copy(itv.begin(), itv.end(), std::ostream_iterator<M4::Handle>(std::cerr, "\n"));
    assert(std::equal(itv.begin(), itv.end(), expected.begin()));
}

static void test_prev_handles_graph_to_trie() {
    auto tg = M4::triegraph_from_graph(
            build_graph(),
            M4::Settings()
                .add_reverse_complement(false)
                .trie_depth(4));

    auto h = tg.prev_trie_handles(M4::Handle(3, 0));
    auto expected = std::vector<M4::Handle> {
        M4::Kmer::from_str("acgg"),
        M4::Kmer::from_str("cacg"),
    };
    std::cerr << "expected" << std::endl;
    std::copy(expected.begin(), expected.end(),
            std::ostream_iterator<M4::Handle>(std::cerr, "\n"));
    std::cerr << "actual" << std::endl;
    std::copy(h.begin(), h.end(), std::ostream_iterator<M4::Handle>(std::cerr, "\n"));
    // std::cerr << M4::Handle(M4::Kmer::from_str("acgg")).is_trie() << std::endl;
    // std::cerr << M4::Handle(M4::Kmer::from_str("cacg")).is_trie() << std::endl;
    // std::cerr << (M4::Kmer::from_str("acgg").data & M4::Kmer::ON_MASK) << std::endl;
    assert(std::equal(h.begin(), h.end(), expected.begin()));
}

static void test_prev_handles_trie() {
    auto tg = M4::triegraph_from_graph(
            build_graph(),
            M4::Settings()
                .add_reverse_complement(false)
                .trie_depth(4));

    auto h = tg.prev_trie_handles(M4::Kmer::from_str("acac"));
    auto expected = std::vector<M4::Handle> { M4::Kmer::from_str("aca") };
    // std::copy(h.begin(), h.end(), std::ostream_iterator<M4::Handle>(std::cerr, "\n"));
    assert(std::equal(h.begin(), h.end(), expected.begin()));
}

static void test_up_handle_trie() {
    auto tg = M4::triegraph_from_graph(
            build_graph(),
            M4::Settings()
                .add_reverse_complement(false)
                .trie_depth(4));

    auto handle = tg.up_trie_handle(M4::Kmer::from_str("acac"));
    assert(handle.is_trie() && handle.kmer == M4::Kmer::from_str("aca"));
}

static void test_next_match_many() {
    auto graph = M4::Graph::Builder()
        .add_node(M4::Str("acgtacgtacgt"), "s1")
        .add_node(M4::Str("ttt"), "s2")
        .add_edge("s1", "s2")
        .build();

    auto tg = M4::triegraph_from_graph(
            std::move(graph),
            M4::Settings()
                .add_reverse_complement(false)
                .trie_depth(4));

    assert(tg.next_match_many(M4::Handle(0, 0), M4::Str("acgtacgtacgt")) == 12);
    assert(tg.next_match_many(M4::Handle(0, 11), M4::Str("ttt")) == 1);
    assert(tg.next_match_many(M4::Handle(0, 12), M4::Str("ttt")) == 0);
    assert(tg.next_match_many(M4::Handle(), M4::Str("acg")) == 0); // invalid
    assert(tg.next_match_many(M4::Kmer::from_str("acg"), M4::Str("acg")) == 0); // trie
}

static void test_exact_short_match() {
    auto tg = M4::triegraph_from_graph(
            build_graph(),
            M4::Settings()
                .add_reverse_complement(false)
                .trie_depth(4));

    assert(tg.exact_short_match(M4::Str("aca")) == M4::Handle(M4::Kmer::from_str("aca")));
    assert(tg.exact_short_match(M4::Str("acac")) == M4::Handle(M4::Kmer::from_str("acac")));
    assert(tg.exact_short_match(M4::Str("ttt")) == M4::Handle::invalid());
    assert(tg.exact_short_match(M4::Str("acacg")) == M4::Handle::invalid());
}

} /* unnamed namespace */

int main() {
    test_next_edit_edges_fwd();
    test_next_edit_edges_split();
    test_next_edit_edges_trie_inner();
    test_next_edit_edges_trie_edge();
    test_next_edit_edges_trie_to_graph();

    test_prev_handles_linear();
    test_prev_handles_split();
    test_prev_handles_graph_to_trie();
    test_prev_handles_trie();
    test_up_handle_trie();

    test_next_match_many();
    test_exact_short_match();

    return 0;
}
