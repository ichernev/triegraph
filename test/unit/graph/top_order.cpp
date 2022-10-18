// SPDX-License-Identifier: MPL-2.0
/*
 * Copyright (c) 2021, Iskren Chernev
 */

#include "triegraph/graph/top_order.h"

#include "testlib/dna.h"
#include "testlib/test.h"

using TG = test::Manager_RK;

int m = test::define_module(__FILE__, [] {

test::define_test("simple linear", [] {
    auto graph = TG::Graph::Builder({
            .add_reverse_complement = false, .add_extends = false })
        .add_node(TG::Str("a"), "s1")
        .add_node(TG::Str("a"), "s2")
        .add_node(TG::Str("a"), "s3")
        .add_edge("s1", "s2")
        .add_edge("s2", "s3")
        .build();


    auto top_ord = triegraph::TopOrder<TG::Graph>::Builder(graph)
        .build();

    top_ord.sanity_check(graph, true);
});

test::define_test("simple dag", [] {
    auto graph = TG::Graph::Builder({
            .add_reverse_complement = false, .add_extends = false })
        .add_node(TG::Str("a"), "s1")
        .add_node(TG::Str("a"), "s2")
        .add_node(TG::Str("a"), "s3")
        .add_node(TG::Str("a"), "s4")
        .add_edge("s1", "s2")
        .add_edge("s1", "s3")
        .add_edge("s2", "s4")
        .add_edge("s3", "s4")
        .build();


    auto top_ord = triegraph::TopOrder<TG::Graph>::Builder(graph)
        .build();

    top_ord.sanity_check(graph, true);
});

test::define_test("simple cyclic", [] {
    auto graph = TG::Graph::Builder({
            .add_reverse_complement = false, .add_extends = false })
        .add_node(TG::Str("a"), "s1")
        .add_node(TG::Str("a"), "s2")
        .add_edge("s1", "s2")
        .add_edge("s2", "s1")
        .build();

    auto top_ord = triegraph::TopOrder<TG::Graph>::Builder(graph)
        .build();

    top_ord.sanity_check(graph, false);
    assert(top_ord.idx[0] == 1);
    assert(top_ord.idx[1] == 0);
});

test::define_test("simple cyclic 2", [] {
    auto graph = TG::Graph::Builder({
            .add_reverse_complement = false, .add_extends = false })
        // define nodes in rev order to test if it properly finds the order
        .add_node(TG::Str("a"), "s4")
        .add_node(TG::Str("a"), "s3")
        .add_node(TG::Str("a"), "s2")
        .add_node(TG::Str("a"), "s1")
        .add_edge("s1", "s2")
        .add_edge("s2", "s3")
        .add_edge("s3", "s2")
        .add_edge("s3", "s4")
        .build();

    auto top_ord = triegraph::TopOrder<TG::Graph>::Builder(graph)
        .build();

    top_ord.sanity_check(graph, false);
    assert(top_ord.idx[0] == 0);
    assert(top_ord.idx[1] == 2);
    assert(top_ord.idx[2] == 1);
    assert(top_ord.idx[3] == 3);
});

});
