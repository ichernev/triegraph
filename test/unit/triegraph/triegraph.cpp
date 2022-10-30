// SPDX-License-Identifier: MPL-2.0
/*
 * Copyright (c) 2021, Iskren Chernev
 */

#include "triegraph/dna_config.h"
#include "triegraph/manager.h"

#include <ranges>

#include "testlib/test.h"
#include "testlib/triegraph/builder.h"

using namespace triegraph;

namespace {

using TG = Manager<dna::DnaConfig<0>>;
using Str = TG::Str;
using EditEdge = TG::EditEdge;
using NodePos = TG::NodePos;
using DnaLetters = TG::Letters;
using Kmer = TG::Kmer;

TG::Graph build_graph() {
    return TG::Graph::Builder({ .add_reverse_complement = false })
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

int m = test::define_module(__FILE__, [] {

test::define_test("next_edit_edges_fwd", [] {
    auto tg = test::tg_from_graph<TG>(build_graph(), 4);

    auto h = tg.next_edit_edges(TG::Handle(2, 1));
    auto expected = std::vector<TG::EditEdge> {
        EditEdge { NodePos { 2, 2 }, EditEdge::SUB,   DnaLetters::A },
        EditEdge { NodePos { 2, 2 }, EditEdge::MATCH, DnaLetters::C },
        EditEdge { NodePos { 2, 2 }, EditEdge::SUB,   DnaLetters::G },
        EditEdge { NodePos { 2, 2 }, EditEdge::SUB,   DnaLetters::T },
        EditEdge { NodePos { 2, 2 }, EditEdge::SUB,   DnaLetters::N },

        EditEdge { NodePos { 2, 1 }, EditEdge::INS,   DnaLetters::A },
        EditEdge { NodePos { 2, 1 }, EditEdge::INS,   DnaLetters::C },
        EditEdge { NodePos { 2, 1 }, EditEdge::INS,   DnaLetters::G },
        EditEdge { NodePos { 2, 1 }, EditEdge::INS,   DnaLetters::T },
        EditEdge { NodePos { 2, 1 }, EditEdge::INS,   DnaLetters::N },

        EditEdge { NodePos { 2, 2 }, EditEdge::DEL,   DnaLetters::EPS },
    };

    assert(std::ranges::equal(h, expected));
});

test::define_test("next_edit_edges_split", [] {
    auto tg = test::tg_from_graph<TG>(build_graph(), 4);
    // auto tg = TG::triegraph_from_graph(
    //         build_graph(),
    //         TG::Settings {
    //             .add_reverse_complement = false,
    //             .trie_depth = 4 });

    auto h = tg.next_edit_edges(TG::Handle(0, 1));
    auto expected = std::vector<TG::EditEdge> {
        EditEdge { NodePos { 2, 0 }, EditEdge::SUB,   DnaLetters::A },
        EditEdge { NodePos { 2, 0 }, EditEdge::MATCH, DnaLetters::C },
        EditEdge { NodePos { 2, 0 }, EditEdge::SUB,   DnaLetters::G },
        EditEdge { NodePos { 2, 0 }, EditEdge::SUB,   DnaLetters::T },
        EditEdge { NodePos { 2, 0 }, EditEdge::SUB,   DnaLetters::N },
        EditEdge { NodePos { 2, 0 }, EditEdge::DEL,   DnaLetters::EPS },

        EditEdge { NodePos { 1, 0 }, EditEdge::SUB,   DnaLetters::A },
        EditEdge { NodePos { 1, 0 }, EditEdge::MATCH, DnaLetters::C },
        EditEdge { NodePos { 1, 0 }, EditEdge::SUB,   DnaLetters::G },
        EditEdge { NodePos { 1, 0 }, EditEdge::SUB,   DnaLetters::T },
        EditEdge { NodePos { 1, 0 }, EditEdge::SUB,   DnaLetters::N },
        EditEdge { NodePos { 1, 0 }, EditEdge::DEL,   DnaLetters::EPS },

        EditEdge { NodePos { 0, 1 }, EditEdge::INS,   DnaLetters::A },
        EditEdge { NodePos { 0, 1 }, EditEdge::INS,   DnaLetters::C },
        EditEdge { NodePos { 0, 1 }, EditEdge::INS,   DnaLetters::G },
        EditEdge { NodePos { 0, 1 }, EditEdge::INS,   DnaLetters::T },
        EditEdge { NodePos { 0, 1 }, EditEdge::INS,   DnaLetters::N },
    };

    // std::ranges::copy(h, std::ostream_iterator<EditEdge>(std::cerr, "\n"));
    assert(std::ranges::equal(h, expected));
});

test::define_test("next_edit_edges_trie_inner", [] {
    auto tg = test::tg_from_graph<TG>(build_graph(), 4);
    // auto tg = TG::triegraph_from_graph(
    //         build_graph(),
    //         TG::Settings {
    //             .add_reverse_complement = false,
    //             .trie_depth = 4 });

    auto h = tg.next_edit_edges(TG::Handle(Kmer::from_str("cg")));

    auto expected = std::vector<TG::EditEdge> {
        EditEdge { Kmer::from_str("cga"), EditEdge::MATCH, DnaLetters::A },
        EditEdge { Kmer::from_str("cga"), EditEdge::SUB, DnaLetters::C },
        EditEdge { Kmer::from_str("cga"), EditEdge::SUB, DnaLetters::G },
        EditEdge { Kmer::from_str("cga"), EditEdge::SUB, DnaLetters::T },
        EditEdge { Kmer::from_str("cga"), EditEdge::SUB, DnaLetters::N },
        EditEdge { Kmer::from_str("cga"), EditEdge::DEL, DnaLetters::EPS },

        EditEdge { Kmer::from_str("cgg"), EditEdge::SUB, DnaLetters::A },
        EditEdge { Kmer::from_str("cgg"), EditEdge::SUB, DnaLetters::C },
        EditEdge { Kmer::from_str("cgg"), EditEdge::MATCH, DnaLetters::G },
        EditEdge { Kmer::from_str("cgg"), EditEdge::SUB, DnaLetters::T },
        EditEdge { Kmer::from_str("cgg"), EditEdge::SUB, DnaLetters::N },
        EditEdge { Kmer::from_str("cgg"), EditEdge::DEL, DnaLetters::EPS },

        EditEdge { Kmer::from_str("cg"), EditEdge::INS, DnaLetters::A },
        EditEdge { Kmer::from_str("cg"), EditEdge::INS, DnaLetters::C },
        EditEdge { Kmer::from_str("cg"), EditEdge::INS, DnaLetters::G },
        EditEdge { Kmer::from_str("cg"), EditEdge::INS, DnaLetters::T },
        EditEdge { Kmer::from_str("cg"), EditEdge::INS, DnaLetters::N },
    };

    // std::ranges::copy(h, std::ostream_iterator<EditEdge>(std::cerr, "\n"));
    assert(std::ranges::equal(h, expected));
});

test::define_test("next_edit_edges_trie_edge", [] {
    auto tg = test::tg_from_graph<TG>(build_graph(), 4);
    // auto tg = TG::triegraph_from_graph(
    //         build_graph(),
    //         TG::Settings {
    //             .add_reverse_complement = false,
    //             .trie_depth = 4 });

    auto h = tg.next_edit_edges(TG::Handle(Kmer::from_str("cga")));

    auto expected = std::vector<TG::EditEdge> {
        EditEdge { Kmer::from_str("cgac"), EditEdge::SUB, DnaLetters::A },
        EditEdge { Kmer::from_str("cgac"), EditEdge::MATCH, DnaLetters::C },
        EditEdge { Kmer::from_str("cgac"), EditEdge::SUB, DnaLetters::G },
        EditEdge { Kmer::from_str("cgac"), EditEdge::SUB, DnaLetters::T },
        EditEdge { Kmer::from_str("cgac"), EditEdge::SUB, DnaLetters::N },
        EditEdge { Kmer::from_str("cgac"), EditEdge::DEL, DnaLetters::EPS },

        EditEdge { Kmer::from_str("cga"), EditEdge::INS, DnaLetters::A },
        EditEdge { Kmer::from_str("cga"), EditEdge::INS, DnaLetters::C },
        EditEdge { Kmer::from_str("cga"), EditEdge::INS, DnaLetters::G },
        EditEdge { Kmer::from_str("cga"), EditEdge::INS, DnaLetters::T },
        EditEdge { Kmer::from_str("cga"), EditEdge::INS, DnaLetters::N },
    };

    // std::ranges::copy(h, std::ostream_iterator<EditEdge>(std::cerr, "\n"));
    assert(std::ranges::equal(h, expected));
});

test::define_test("next_edit_edges_trie_to_graph", [] {
    auto tg = test::tg_from_graph<TG>(build_graph(), 4);
    // auto tg = TG::triegraph_from_graph(
    //         build_graph(),
    //         TG::Settings {
    //             .add_reverse_complement = false,
    //             .trie_depth = 4 });

    auto h = tg.next_edit_edges(TG::Handle(Kmer::from_str("acgg")));

    // auto er = tg.data.trie_data().trie2graph.equal_range(Kmer::from_str("acgg"));
    // for (auto it = er.first; it != er.second; ++it) {
    //     std::cerr << it->second << std::endl;
    // }

    auto expected = std::vector<TG::EditEdge> {
        EditEdge { NodePos { 3, 1 }, EditEdge::MATCH, DnaLetters::A },
        EditEdge { NodePos { 3, 1 }, EditEdge::SUB,   DnaLetters::C },
        EditEdge { NodePos { 3, 1 }, EditEdge::SUB,   DnaLetters::G },
        EditEdge { NodePos { 3, 1 }, EditEdge::SUB,   DnaLetters::T },
        EditEdge { NodePos { 3, 1 }, EditEdge::SUB,   DnaLetters::N },

        EditEdge { NodePos { 3, 0 }, EditEdge::INS,   DnaLetters::A },
        EditEdge { NodePos { 3, 0 }, EditEdge::INS,   DnaLetters::C },
        EditEdge { NodePos { 3, 0 }, EditEdge::INS,   DnaLetters::G },
        EditEdge { NodePos { 3, 0 }, EditEdge::INS,   DnaLetters::T },
        EditEdge { NodePos { 3, 0 }, EditEdge::INS,   DnaLetters::N },

        EditEdge { NodePos { 3, 1 }, EditEdge::DEL,   DnaLetters::EPS },
    };

    // std::ranges::copy(h, std::ostream_iterator<EditEdge>(std::cerr, "\n"));
    assert(std::ranges::equal(h, expected));
});

test::define_test("prev_handles_linear", [] {
    auto tg = test::tg_from_graph<TG>(build_graph(), 4);
    // auto tg = TG::triegraph_from_graph(
    //         build_graph(),
    //         TG::Settings {
    //             .add_reverse_complement = false,
    //             .trie_depth = 4 });

    auto h = tg.prev_graph_handles(TG::Handle(0, 1));
    // auto hh = h.begin();
    // std::cerr << hh.single.h << std::endl;
    // std::cerr << hh.single.has_more() << std::endl;
    // ++hh;
    // std::cerr << hh.single.h << std::endl;
    // std::cerr << hh.single.has_more() << std::endl;
    // for (const auto &x: h) {
    //     std::cerr << "HHH " << std::endl;
    // }
    auto expected = std::vector<TG::Handle> { { 0, 0 } };
    assert(std::ranges::equal(h, expected));

    // auto itv = _collect_it(tg.prev_graph_handles_it(TG::Handle(0, 1)));

    assert(std::ranges::equal(tg.prev_graph_handles(TG::Handle(0, 1)), expected));
});

test::define_test("prev_handles_split", [] {
    auto tg = test::tg_from_graph<TG>(build_graph(), 4);
    // auto tg = TG::triegraph_from_graph(
    //         build_graph(),
    //         TG::Settings {
    //             .add_reverse_complement = false,
    //             .trie_depth = 4 });

    auto h = tg.prev_graph_handles(TG::Handle(3, 0));
    auto expected = std::vector<TG::Handle> { { 2, 2 }, { 1, 1 } };
    // std::ranges::copy(h, std::ostream_iterator<TG::Handle>(std::cerr, "\n"));
    assert(std::ranges::equal(h, expected));

    // auto itv = _collect_it();
    // std::ranges::copy(itv, std::ostream_iterator<TG::Handle>(std::cerr, "\n"));
    assert(std::ranges::equal(tg.prev_graph_handles(TG::Handle(3, 0)), expected));
});

test::define_test("prev_handles_graph_to_trie", [] {
    auto tg = test::tg_from_graph<TG>(build_graph(), 4);
    // auto tg = TG::triegraph_from_graph(
    //         build_graph(),
    //         TG::Settings {
    //             .add_reverse_complement = false,
    //             .trie_depth = 4 });

    auto h = tg.prev_trie_handles(TG::Handle(3, 0));
    auto expected = std::vector<TG::Handle> {
        TG::Kmer::from_str("acgg"),
        TG::Kmer::from_str("cacg"),
    };
    // std::cerr << "expected" << std::endl;
    // std::copy(expected.begin(), expected.end(),
    //         std::ostream_iterator<TG::Handle>(std::cerr, "\n"));
    // std::cerr << "actual" << std::endl;
    // std::ranges::copy(h, std::ostream_iterator<TG::Handle>(std::cerr, "\n"));
    // std::cerr << TG::Handle(TG::Kmer::from_str("acgg")).is_trie() << std::endl;
    // std::cerr << TG::Handle(TG::Kmer::from_str("cacg")).is_trie() << std::endl;
    // std::cerr << (TG::Kmer::from_str("acgg").data & TG::Kmer::ON_MASK) << std::endl;
    assert(test::equal_sorted(h, expected));
});

test::define_test("prev_handles_trie", [] {
    auto tg = test::tg_from_graph<TG>(build_graph(), 4);
    // auto tg = TG::triegraph_from_graph(
    //         build_graph(),
    //         TG::Settings {
    //             .add_reverse_complement = false,
    //             .trie_depth = 4 });

    auto h = tg.prev_trie_handles(TG::Kmer::from_str("acac"));
    auto expected = std::vector<TG::Handle> { TG::Kmer::from_str("aca") };
    // std::ranges::copy(h, std::ostream_iterator<TG::Handle>(std::cerr, "\n"));
    assert(test::equal_sorted(h, expected));
});

test::define_test("up_handle_trie", [] {
    auto tg = test::tg_from_graph<TG>(build_graph(), 4);
    // auto tg = TG::triegraph_from_graph(
    //         build_graph(),
    //         TG::Settings {
    //             .add_reverse_complement = false,
    //             .trie_depth = 4 });

    auto handle = tg.up_trie_handle(TG::Kmer::from_str("acac"));
    assert(handle.is_trie() && handle.kmer() == TG::Kmer::from_str("aca"));
});

test::define_test("next_match_many", [] {
    auto graph = TG::Graph::Builder({ .add_reverse_complement = false })
        .add_node(TG::Str("acgtacgtacgt"), "s1")
        .add_node(TG::Str("ttt"), "s2")
        .add_edge("s1", "s2")
        .build();

    auto tg = test::tg_from_graph<TG>(std::move(graph), 4);
    // auto tg = TG::triegraph_from_graph(
    //         std::move(graph),
    //         TG::Settings {
    //             .add_reverse_complement = false,
    //             .trie_depth = 4 });

    assert(tg.next_match_many(TG::Handle(0, 0), TG::Str("acgtacgtacgt")) == 12);
    assert(tg.next_match_many(TG::Handle(0, 11), TG::Str("ttt")) == 1);
    assert(tg.next_match_many(TG::Handle(0, 12), TG::Str("ttt")) == 0);
    assert(tg.next_match_many(TG::Handle(), TG::Str("acg")) == 0); // invalid
    assert(tg.next_match_many(TG::Kmer::from_str("acg"), TG::Str("acg")) == 0); // trie
});

test::define_test("exact_short_match", [] {
    auto tg = test::tg_from_graph<TG>(build_graph(), 4);
    // auto tg = TG::triegraph_from_graph(
    //         build_graph(),
    //         TG::Settings {
    //             .add_reverse_complement = false,
    //             .trie_depth = 4 });

    assert(tg.exact_short_match(TG::Str("aca")) == TG::Handle(TG::Kmer::from_str("aca")));
    assert(tg.exact_short_match(TG::Str("acac")) == TG::Handle(TG::Kmer::from_str("acac")));
    assert(tg.exact_short_match(TG::Str("ttt")) == TG::Handle::invalid());
    assert(tg.exact_short_match(TG::Str("acacg")) == TG::Handle::invalid());
});

test::define_test("reverse_handle", [] {
    auto tg = test::tg_from_graph<TG>(
            TG::Graph::Builder({ .add_reverse_complement = true })
                .add_node(Str("acgt"), "s1")
                .add_node(Str("cc"), "s2")
                .add_edge("s1", "s2")
                .build(),
            4);

    assert(tg.reverse(TG::Handle(0, 0)) == TG::Handle(1, 3));
    assert(tg.reverse(TG::Handle(0, 1)) == TG::Handle(1, 2));
    assert(tg.reverse(TG::Handle(0, 2)) == TG::Handle(1, 1));
    assert(tg.reverse(TG::Handle(0, 3)) == TG::Handle(1, 0));
});

});

} /* unnamed namespace */

