// SPDX-License-Identifier: MPL-2.0
/*
 * Copyright (c) 2021, Iskren Chernev
 */

#include "testlib/dna.h"
#include "testlib/test.h"
#include "testlib/trie/builder/tester.h"

using TG = test::Manager_RK;

int m = test::define_module(__FILE__, [] {
    using Tester = test::TrieBuilderTester<TG, TG::TrieBuilderNBFS>;
    Tester::define_tests();

    test::define_test("simple linear", [] {
        auto graph = TG::Graph::Builder({
                .add_reverse_complement = false })
            .add_node(TG::Str("ac"), "s1")
            .add_node(TG::Str("gt"), "s2")
            .add_node(TG::Str("at"), "s3")
            .add_edge("s1", "s2")
            .add_edge("s2", "s3")
            .build();

        auto pairs = Tester::graph_to_pairs(graph, {}, 2);
        // TG::init({
        //     .add_reverse_complement = false,
        //     .trie_depth = 2
        // });
        // auto lloc = TG::LetterLocData(graph);
        // auto pairs = TG::VectorPairs();
        // TG::TrieBuilderPBFS(graph, lloc, pairs)
        //     .compute_pairs(lloc);

        assert(std::ranges::equal(
                    pairs.sort_by_rev().unique().fwd_pairs(),
                    typename decltype(pairs)::fwd_vec {
            { TG::Kmer::from_str("ac"), 2 },
            { TG::Kmer::from_str("cg"), 3 },
            { TG::Kmer::from_str("gt"), 4 },
            { TG::Kmer::from_str("ta"), 5 },
            { TG::Kmer::from_str("at"), 6 },
        }));
    });

    test::define_test("simple linear 2", [] {
        auto graph = TG::Graph::Builder({
                .add_reverse_complement = false })
            .add_node(TG::Str("ac"), "s1")
            .add_node(TG::Str("gt"), "s2")
            .add_node(TG::Str("at"), "s3")
            .add_edge("s1", "s2")
            .add_edge("s2", "s3")
            .build();

        // trie_depth = 3 will trigger second case in bfs
        auto pairs = Tester::graph_to_pairs(graph, {}, 3);

        // std::ranges::sort(pairs, [](const auto &a, const auto &b) {
        //         return a.second < b.second; });
        // auto sr = std::ranges::unique(pairs);
        // pairs.resize(sr.begin() - pairs.begin());

        // std::ranges::copy(pairs, std::ostream_iterator<TG::vec_pairs::value_type>(std::cerr, " "));
        // std::cerr << "----" << std::endl;
        // for (const auto &p : pairs) {
        //     std::cerr << p.first << " " << p.second << std::endl;
        // }
        // std::cerr << "----" << std::endl;
        assert(std::ranges::equal(
                    pairs.sort_by_rev().unique().fwd_pairs(),
                    typename decltype(pairs)::fwd_vec {
            { TG::Kmer::from_str("acg"), 3 },
            { TG::Kmer::from_str("cgt"), 4 },
            { TG::Kmer::from_str("gta"), 5 },
            { TG::Kmer::from_str("tat"), 6 },
        }));
    });

    test::define_test("simple dag", [] {
        auto graph = TG::Graph::Builder({
                .add_reverse_complement = false })
            .add_node(TG::Str("a"), "s1")
            .add_node(TG::Str("cg"), "s2")
            .add_node(TG::Str("t"), "s3")
            .add_node(TG::Str("ac"), "s4")
            .add_edge("s1", "s2")
            .add_edge("s1", "s3")
            .add_edge("s2", "s4")
            .add_edge("s3", "s4")
            .build();

        auto pairs = Tester::graph_to_pairs(graph, {}, 4);

        assert(std::ranges::equal(
                    pairs.sort_by_rev().unique().fwd_pairs(),
                    typename decltype(pairs)::fwd_vec {
            { TG::Kmer::from_str("acga"), 5 },
            { TG::Kmer::from_str("atac"), 6 },
            { TG::Kmer::from_str("cgac"), 6 },
        }));
    });

});
