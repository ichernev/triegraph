// SPDX-License-Identifier: MPL-2.0
/*
 * Copyright (c) 2021, Iskren Chernev
 */

#include <iterator>

#include "testlib/dna.h"
#include "testlib/test.h"

using TG = test::Manager_RK;

int m = test::define_module(__FILE__, [] {

test::define_test("iter_concept", [] {
    auto func = [](std::forward_iterator auto it) {};
    func(TG::SparseStarts::const_iterator());
});

test::define_test("small_linear", [] {
    auto g = TG::Graph::Builder({ .add_reverse_complement = false, .add_extends = false })
        .add_node(TG::Str("acgtacgt"), "s1")
        .build();

    auto ss = TG::SparseStarts(g);
    auto starts = ss.compute_starts_every(2, {0});

    // std::ranges::copy(starts, std::ostream_iterator<TG::NodePos>(std::cerr, " ")); std::cerr << std::endl;
    assert(std::ranges::equal(starts,
                std::vector<TG::NodePos> { {0, 0}, {0, 2}, {0, 4}, {0, 6} }));
});

test::define_test("small_split", [] {
    auto g = TG::Graph::Builder({ .add_reverse_complement = false, .add_extends = false })
        .add_node(TG::Str("aaa"), "s1")
        .add_node(TG::Str("a"), "s2")
        .add_node(TG::Str("aa"), "s3")
        .add_node(TG::Str("a"), "s4")
        .add_edge("s1", "s2")
        .add_edge("s1", "s3")
        .add_edge("s2", "s4")
        .add_edge("s3", "s4")
        .build();

    auto ss = TG::SparseStarts(g);
    auto starts = ss.compute_starts_every(2, {0});

    // std::ranges::copy(starts, std::ostream_iterator<TG::NodePos>(std::cerr, " ")); std::cerr << std::endl;
    assert(std::ranges::equal(starts,
                std::vector<TG::NodePos> { {0, 0}, {0, 2}, {2, 1}, {3, 0} }));
});

test::define_test("small_split2", [] {
    auto g = TG::Graph::Builder({
            .add_reverse_complement = false,
            .add_extends = false })
        .add_node(TG::Str("acgt"), "s1")
        .add_node(TG::Str("acgta"), "s2")
        .add_node(TG::Str("aa"), "s3")
        .add_edge("s1", "s3")
        .add_edge("s2", "s3")
        .build();

    auto ss = TG::SparseStarts(g);
    auto starts = ss.compute_starts_every(3, {0, 1});

    // std::ranges::copy(starts, std::ostream_iterator<TG::NodePos>(std::cerr, " ")); std::cerr << std::endl;
    assert(std::ranges::equal(starts,
            std::vector<TG::NodePos> { {0, 0}, {0, 3}, {1, 0}, {1, 3}, {2, 1} }));
});

test::define_test("small_loop", [] {
    auto g = TG::Graph::Builder({ .add_reverse_complement = false, .add_extends = false })
        .add_node(TG::Str("acgt"), "s1")
        .add_node(TG::Str("acgta"), "s2")
        .add_node(TG::Str("aa"), "s3")
        .add_edge("s1", "s3")
        .add_edge("s2", "s3")
        .add_edge("s3", "s3")
        .build();

    auto ss = TG::SparseStarts(g);
    auto starts = ss.compute_starts_every(3, {0, 1});

    // std::ranges::copy(starts, std::ostream_iterator<TG::NodePos>(std::cerr, " ")); std::cerr << std::endl;
    assert(std::ranges::equal(starts,
            std::vector<TG::NodePos> { {0, 0}, {0, 3}, {1, 0}, {1, 3}, {2, 0} }));
});

});
