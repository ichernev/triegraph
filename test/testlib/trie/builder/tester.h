// SPDX-License-Identifier: MPL-2.0
/*
 * Copyright (c) 2021, Iskren Chernev
 */

#ifndef __TESTLIB_TRIE_BUILDER_TESTER_H__
#define __TESTLIB_TRIE_BUILDER_TESTER_H__

#include <algorithm>
#include <ranges>

namespace test {

template <typename TG, typename TrieBuilder = TG::TrieBuilder>
struct TrieBuilderTester {
    using Self = TrieBuilderTester;

    using NodePos = TG::NodePos;
    using Kmer = TG::Kmer;
    using Str = TG::Str;

    static TG::VectorPairs graph_to_pairs(
            const TG::Graph &g,
            TrieBuilder::Settings s = {},
            typename TG::Kmer::klen_type trie_depth = 4) {
        auto lloc = typename TG::LetterLocData(g);
        auto ks = TG::KmerSettings::template from_depth<typename TG::KmerHolder>(trie_depth);
        return TG::template graph_to_pairs<TrieBuilder>(
                g, lloc, std::move(ks), std::move(s), lloc);
    }

    static void tiny_linear_graph() {
        auto g = typename TG::Graph::Builder({ .add_reverse_complement = false })
            .add_node(Str("acgtacgtac"), "s1")
            .build();

        auto pairs = graph_to_pairs(g);

        // std::cerr << std::endl;
        // for (const auto &p : pairs.fwd_pairs()) {
        //     std::cerr << p.first << " " << p.second << std::endl;
        // }
        // std::cerr << pairs.size() << std::endl;
        assert(std::ranges::equal(pairs.sort_by_fwd().unique().fwd_pairs(),
                    typename decltype(pairs)::fwd_vec {
            { Kmer::from_str("acgt"), 4 },
            { Kmer::from_str("acgt"), 8 },
            { Kmer::from_str("cgta"), 5 },
            { Kmer::from_str("cgta"), 9 },
            { Kmer::from_str("gtac"), 6 },
            { Kmer::from_str("gtac"), 10 },
            { Kmer::from_str("tacg"), 7 },
        }));
    }

    static void small_nonlinear_graph() {
        auto g = typename TG::Graph::Builder({ .add_reverse_complement = false })
            .add_node(Str("a"), "s1")
            .add_node(Str("cg"), "s2")
            .add_node(Str("t"), "s3")
            .add_node(Str("ac"), "s4")
            .add_edge("s1", "s2")
            .add_edge("s1", "s3")
            .add_edge("s2", "s4")
            .add_edge("s3", "s4")
            .build();


        /****************
         *     12       *
         *     cg       *
         *  a /  \ ac   *
         *  0 \  / 45 6 *
         *     t        *
         *     3        *
         ***************/
        auto pairs = graph_to_pairs(g);

        assert(std::ranges::equal(
                    pairs.sort_by_fwd().unique().fwd_pairs(),
                    typename decltype(pairs)::fwd_vec {
            { Kmer::from_str("acga"), 5 },
            { Kmer::from_str("atac"), 6 },
            { Kmer::from_str("cgac"), 6 },
        }));
    }

    static void multiple_ends() {
        auto g = typename TG::Graph::Builder({ .add_reverse_complement = false })
            .add_node(Str("acg"), "s1")
            .add_node(Str("c"), "s2")
            .add_node(Str("g"), "s3")
            .add_edge("s1", "s2")
            .add_edge("s1", "s3")
            .build();

        /*****************
         *       3   5   *
         *       c - a   *
         *  acg /        *
         *  012 \        *
         *       g - a   *
         *       4   6   *
         *****************/

        auto pairs = graph_to_pairs(g);

        assert(std::ranges::equal(
                    pairs.sort_by_fwd().unique().fwd_pairs(),
                    typename decltype(pairs)::fwd_vec {
            { Kmer::from_str("acgc"), 5 },
            { Kmer::from_str("acgg"), 6 },
        }));
    }

    static void define_tests() {
        test::define_test("tiny_linear_graph", &Self::tiny_linear_graph);
        test::define_test("small_nonlinear_graph", &Self::small_nonlinear_graph);
        test::define_test("multiple_ends", &Self::multiple_ends);
    }
};

} /* namespace test */

#endif /* __TESTLIB_TRIE_BUILDER_TESTER_H__ */
