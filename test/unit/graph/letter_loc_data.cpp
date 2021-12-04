// SPDX-License-Identifier: MPL-2.0
/*
 * Copyright (c) 2021, Iskren Chernev
 */

#include <utility>
#include <vector>

#include "testlib/dna.h"
#include "testlib/test.h"

struct LLNoIdx : public test::DnaConfig_RK {
    static constexpr int LetterLocIdxShift = -1;
};

struct LLWithIdx : public test::DnaConfig_RK {
    static constexpr int LetterLocIdxShift = 2;
};

using TG = triegraph::Manager<LLNoIdx>;

static TG::Graph build_graph() {
    return TG::Graph::Builder({ .add_reverse_complement = false })
        .add_node(TG::Str("acgt"), "s1")
        .add_node(TG::Str("a"), "s2")
        .add_node(TG::Str("cgt"), "s3")
        .add_node(TG::Str("aa"), "s4")
        .add_edge("s1", "s2")
        .add_edge("s1", "s3")
        .add_edge("s2", "s4")
        .add_edge("s3", "s4")
        .build();

     /*******************
      *       [1]       *
      *  [0]  A   [3][4]*
      *  ACGT/4  \ AA-a *
      *  0123\[2]/ 89 9 *
      *       CGT       *
      *       567       *
      ******************/
}

static std::vector<std::pair<TG::LetterLoc, TG::NodePos>> get_expected() {
    return std::vector<std::pair<TG::LetterLoc, TG::NodePos>> {
        { 0,  { 0,  0 } },
        { 1,  { 0,  1 } },
        { 2,  { 0,  2 } },
        { 3,  { 0,  3 } },
        { 4,  { 1,  0 } },
        { 5,  { 2,  0 } },
        { 6,  { 2,  1 } },
        { 7,  { 2,  2 } },
        { 8,  { 3,  0 } },
        { 9,  { 3,  1 } },
        { 10, { 4,  0 } },
    };
}

int m = test::define_module(__FILE__, [] {

test::define_test("lloc_no_idx", [] {
    auto g = build_graph();
    auto ll = TG::LetterLocData(g);
    auto expected = get_expected();

    assert(ll.num_locations == 11);

    for (auto e : expected) {
        // std::cerr << e.first << " " << e.second << std::endl;
        // std::cerr << ll.expand(e.first) << " " << ll.compress(e.second) << std::endl;
        assert(ll.expand(e.first) == e.second);
        assert(ll.compress(e.second) == e.first);
    }
});

test::define_test("lloc_with_idx", [] {
    using TG2 = triegraph::Manager<LLWithIdx>;

    auto g = build_graph();
    auto ll = TG2::LetterLocData(g);
    auto expected = get_expected();

    assert(ll.num_locations == 11);

    for (auto e : expected) {
        // std::cerr << e.first << " " << e.second << std::endl;
        // std::cerr << ll.expand(e.first) << " " << ll.compress(e.second) << std::endl;
        assert(ll.expand(e.first) == e.second);
        assert(ll.compress(e.second) == e.first);
    }
});

test::define_test("lloc_iter", [] {
    auto g = build_graph();
    auto ll = TG::LetterLocData(g);
    auto expected = get_expected();

    // std::ranges::copy(ll, std::ostream_iterator<TG::NodePos>(std::cerr, "\n"));
    assert(std::ranges::equal(
                expected | std::ranges::views::transform(
                    &std::pair<TG::LetterLoc, TG::NodePos>::second
                    /*[](const auto &p) { return p.second; }*/),
                ll));
});

test::define_test("reverse", [] {
    auto g = TG::Graph::Builder({ .add_reverse_complement = true, .add_extends = false })
        .add_node(TG::Str("acgt"), "s1")
        .add_node(TG::Str("ag"), "s2")
        .add_edge("s1", "s2")
        .build();

    assert(TG::NodePos(0, 0).reverse(g) == TG::NodePos(1, 3));
    assert(TG::NodePos(0, 1).reverse(g) == TG::NodePos(1, 2));
    assert(TG::NodePos(0, 2).reverse(g) == TG::NodePos(1, 1));
    assert(TG::NodePos(0, 3).reverse(g) == TG::NodePos(1, 0));
    assert(TG::NodePos(1, 0).reverse(g) == TG::NodePos(0, 3));
    assert(TG::NodePos(1, 1).reverse(g) == TG::NodePos(0, 2));
    assert(TG::NodePos(1, 2).reverse(g) == TG::NodePos(0, 1));
    assert(TG::NodePos(1, 3).reverse(g) == TG::NodePos(0, 0));
    for (TG::NodeLoc n = 0; n < g.data.nodes.size(); ++n) {
        for (TG::NodeLen l = 0; l < g.node(n).seg.size(); ++l) {
            assert(TG::NodePos(n, l).reverse(g).reverse(g) == TG::NodePos(n, l));
        }
    }
});

});
