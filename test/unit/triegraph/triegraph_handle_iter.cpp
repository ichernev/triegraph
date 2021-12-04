// SPDX-License-Identifier: MPL-2.0
/*
 * Copyright (c) 2021, Iskren Chernev
 */

#include "testlib/dna.h"
#include "testlib/test.h"

using TG = test::Manager_RK;

int m = test::define_module(__FILE__, [] {

test::define_test("linear", [] {
    TG::kmer_set_depth(15);
    auto g = TG::Graph::Builder()
        .add_node(TG::Str("acg"), "s1")
        .build();

    // testing auto conversion from Iter to Helper
    TG::PrevHandleIterHelper h = TG::PrevHandleIter::make_graph(g, TG::Handle(0, 2));
    auto expected = std::vector<TG::Handle> { { 0, 1 } };

    assert(std::equal(h.begin(), h.end(), expected.begin()));
});

test::define_test("split", [] {
    TG::kmer_set_depth(15);
    auto g = TG::Graph::Builder()
        .add_node(TG::Str("a"), "s1")
        .add_node(TG::Str("c"), "s2")
        .add_node(TG::Str("g"), "s3")
        .add_edge("s1", "s3")
        .add_edge("s2", "s3")
        .build();

    // testing auto conversion from Iter to Helper
    TG::PrevHandleIterHelper h = TG::PrevHandleIter::make_graph(g, TG::Handle(2, 0));
    auto expected = std::vector<TG::Handle> { { 1, 0 }, { 0, 0 } };

    assert(std::equal(h.begin(), h.end(), expected.begin()));
});

});

