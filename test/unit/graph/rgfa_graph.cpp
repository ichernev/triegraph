// SPDX-License-Identifier: MPL-2.0
/*
 * Copyright (c) 2021, Iskren Chernev
 */

#include "triegraph/graph/rgfa_graph.h"
#include "triegraph/alphabet/dna_letter.h"
#include "triegraph/alphabet/str.h"

#include "triegraph/dna_config.h"
#include "triegraph/manager.h"

#include <ranges>
// #include <iostream>
// #include <assert.h>
#include "testlib/test.h"

using namespace triegraph;

using DnaStr = Str<dna::DnaLetter, u32>;

static void func(std::ranges::input_range auto &&) {}

int m = test::define_module(__FILE__, [] {

test::define_test("iter_concept", [] {
    using Graph = RgfaGraph<DnaStr, u32>;
    auto graph = Graph::Builder()
        .add_node(DnaStr("a"), "s1")
        .build();
    func(graph.forward_from(0));
});

test::define_test("settings", [] {
    auto s = RgfaGraph<DnaStr, u32>::Settings{};
    assert(s.add_reverse_complement);
    assert(s.add_extends);

    s = RgfaGraph<DnaStr, u32>::Settings{
        .add_reverse_complement = false
    };
    assert(!s.add_reverse_complement);
    assert(s.add_extends);

    s = RgfaGraph<DnaStr, u32>::Settings{
        .add_reverse_complement = false,
        .add_extends = false
    };
    assert(!s.add_reverse_complement);
    assert(!s.add_extends);
});

test::define_test("small", [] {
    auto graph = RgfaGraph<DnaStr, u32>::from_file("data/simpler.gfa", {
            .add_reverse_complement = false,
            .add_extends = false,
    });

    assert(graph.num_nodes() == 7);
    assert(graph.num_edges() == 8 * 2);
    assert(graph.data.edge_start.size() == graph.num_nodes());
    assert(graph.data.redge_start.size() == graph.num_nodes());

    // std::cerr << graph;

    auto edge_helper = graph.forward_from(0);
    assert(std::ranges::distance(edge_helper) == 2);

    edge_helper = graph.forward_from(1);
    assert(std::ranges::distance(edge_helper) == 2);

    edge_helper = graph.forward_from(2);
    assert(std::ranges::distance(edge_helper) == 1);

    edge_helper = graph.forward_from(3);
    assert(std::ranges::distance(edge_helper) == 0);

    {
        auto nxt = graph.forward_one(2);
        assert(nxt.has_value());
        assert(*nxt == 3);
    }
    {
        auto nxt = graph.forward_one(0);
        assert(!nxt);
    }
    {
        auto nxt = graph.forward_one(6);
        assert(!!nxt);
        assert(*nxt == 5);
    }
});

// test::define_test("pasgal_mhc1", [] {
//     auto graph = RgfaGraph<DnaStr, u32>::from_file("data/pasgal-MHC1.gfa", {
//             .add_reverse_complement = true,
//             .add_extends = true,
//     });
//     using Graph = RgfaGraph<DnaStr, u32>;
//     using NodeLoc = Graph::NodeLoc;
//     using EdgeLoc = Graph::EdgeLoc;
//     constexpr EdgeLoc INV_SIZE = Graph::INV_SIZE;

//     assert(graph.data.nodes.size() == graph.data.edge_start.size());
//     assert(graph.data.nodes.size() == graph.data.redge_start.size());
//     for (NodeLoc i = 0; i < graph.data.nodes.size(); ++i) {
//         assert(graph.data.edge_start[i] == INV_SIZE ||
//                 graph.data.edge_start[i] < graph.data.edges.size());
//         assert(graph.data.redge_start[i] == INV_SIZE ||
//                 graph.data.redge_start[i] < graph.data.edges.size());
//     }
//     for (EdgeLoc i = 0; i < graph.data.edges.size(); ++i) {
//         assert(graph.data.edges[i].to < graph.data.nodes.size());
//         assert(graph.data.edges[i].next == INV_SIZE ||
//                 graph.data.edges[i].next < graph.data.edges.size());
//     }
// });

// using TG = triegraph::Manager<triegraph::dna::DnaConfig<14>>;

// static void test_pasgal_mhc1() {
//     auto tg = TG::triegraph_from_rgfa_file("data/pasgal-MHC1.gfa", TG::Settings());
//     auto [a, b, c] = tg.graph_size();
//     auto [d, e, f] = tg.trie_size();

//     std::cerr << a << " " << b << " " << c << std::endl;
//     std::cerr << d << " " << e << " " << f << std::endl;

//     //auto graph = RgfaGraph<DnaStr, u32>::from_file("data/HG_22_linear.gfa");
// }

// static void test_hg22_linear() {
//     auto tg = TG::triegraph_from_rgfa_file("data/HG_22_linear.gfa", TG::Settings());
//     auto [a, b, c] = tg.graph_size();
//     auto [d, e, f] = tg.trie_size();

//     std::cerr << a << " " << b << " " << c << std::endl;
//     std::cerr << d << " " << e << " " << f << std::endl;

//     //auto graph = RgfaGraph<DnaStr, u32>::from_file("data/HG_22_linear.gfa");
// }

test::define_test("revcomp", [] {
    auto graph = RgfaGraph<DnaStr, u32>::Builder({
            .add_reverse_complement = true, .add_extends = false })
        .add_node(DnaStr("ac"), "s1")
        .add_node(DnaStr("gg"), "s2")
        .add_node(DnaStr("acg"), "s3")
        .add_node(DnaStr("ac"), "s4")
        .add_edge("s1", "s2")
        .add_edge("s1", "s3")
        .add_edge("s2", "s4")
        .add_edge("s3", "s4")
        .build();
    // std::cerr << graph;

    assert(graph.node(1).seg.to_str() == "gt");
    assert(graph.node(3).seg.to_str() == "cc");
    assert(graph.node(5).seg.to_str() == "cgt");
    assert(graph.node(7).seg.to_str() == "gt");

    auto node_collector = [](auto ith) {
        std::vector<u32> res;
        std::ranges::transform(
                ith,
                std::back_inserter(res),
                [](auto e) { return e.node_id; });
        std::ranges::sort(res);
        return res;
    };

    assert((node_collector(graph.forward_from(7)) == std::vector<u32>{3, 5}));
    assert((node_collector(graph.forward_from(5)) == std::vector<u32>{1}));
    assert((node_collector(graph.forward_from(3)) == std::vector<u32>{1}));
    assert((node_collector(graph.forward_from(1)) == std::vector<u32>{}));
});

test::define_test("extends", [] {
    auto graph = RgfaGraph<DnaStr, u32>::Builder({
            .add_reverse_complement = false, .add_extends = true })
        .add_node(DnaStr("ac"), "s1")
        .add_node(DnaStr("gg"), "s2")
        .add_node(DnaStr("acg"), "s3")
        .add_edge("s1", "s2")
        .add_edge("s1", "s3")
        .build();

    // std::cerr << graph << std::endl;

    assert(std::ranges::distance(graph.forward_from(0)) == 2);
    assert(std::ranges::distance(graph.forward_from(1)) == 1);
    assert(std::ranges::distance(graph.forward_from(2)) == 1);
    assert(std::ranges::distance(graph.forward_from(3)) == 0);
    assert(graph.node(3).seg_id == "extend:" + graph.node(1).seg_id);
    assert(std::ranges::distance(graph.forward_from(4)) == 0);
    assert(graph.node(4).seg_id == "extend:" + graph.node(2).seg_id);
});

test::define_test("fasta single", [] {
    auto graph = RgfaGraph<DnaStr, u32>::from_file("data/simple.fasta", {
            .add_reverse_complement = false,
            .add_extends = false });

    // std::cerr << graph;
    assert(graph.num_nodes() == 1);
    assert(graph.node(0).seg_id == "Simple header line");
    assert(graph.node(0).seg == DnaStr("acgt"));
});


test::define_test("fasta multi", [] {
    auto graph = RgfaGraph<DnaStr, u32>::from_file("data/multi.fasta", {
            .add_reverse_complement = false,
            .add_extends = false });

    // std::cerr << graph;
    assert(graph.num_nodes() == 3);
    assert(graph.node(0).seg_id == "Simple header line1");
    assert(graph.node(0).seg == DnaStr("acgt"));

    assert(graph.node(1).seg_id == "Simple header line2");
    assert(graph.node(1).seg == DnaStr("tgca"));

    assert(graph.node(2).seg_id == "Simple header line3");
    assert(graph.node(2).seg == DnaStr("aaaa"));
});

});
