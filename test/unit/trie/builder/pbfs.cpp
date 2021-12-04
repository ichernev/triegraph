// SPDX-License-Identifier: MPL-2.0
/*
 * Copyright (c) 2021, Iskren Chernev
 */

#include "testlib/dna.h"
#include "testlib/test.h"
#include "testlib/trie/builder/tester.h"

using namespace triegraph;

using triegraph::dna::CfgFlags;
using TGX = Manager<dna::DnaConfig<0,
      CfgFlags::ALLOW_INNER_KMER | CfgFlags::VP_RAW_KMERS>>;

using TG = test::Manager_RK;
int m = test::define_module(__FILE__, [] {
    test::TrieBuilderTester<TG, TG::TrieBuilderPBFS>::define_tests();

    test::define_test("cut_early", [] {
    // static void test_cut_early() {
        auto kmer_s = &TGX::Kmer::from_str;
        auto graph = TGX::Graph::Builder({ .add_reverse_complement = false })
            .add_node(TGX::Str("a"), "s00")
            .add_node(TGX::Str("g"), "s01")
            .add_node(TGX::Str("c"), "s10")
            .add_node(TGX::Str("t"), "s11")
            .add_node(TGX::Str("a"), "s20")
            .add_node(TGX::Str("g"), "s21")
            .add_node(TGX::Str("c"), "s30")
            .add_node(TGX::Str("t"), "s31")
            // .add_node(TGX::Str("a"), "s40")
            // .add_node(TGX::Str("g"), "s41")
            .add_edge("s00", "s10")
            .add_edge("s00", "s11")
            .add_edge("s01", "s10")
            .add_edge("s01", "s11")
            .add_edge("s10", "s20")
            .add_edge("s10", "s21")
            .add_edge("s11", "s20")
            .add_edge("s11", "s21")
            .add_edge("s20", "s30")
            .add_edge("s20", "s31")
            .add_edge("s21", "s30")
            .add_edge("s21", "s31")
            // .add_edge("s30", "s40")
            // .add_edge("s30", "s41")
            // .add_edge("s31", "s40")
            // .add_edge("s31", "s41")
            .build();

        auto pairs = test::TrieBuilderTester<TGX, TGX::TrieBuilderPBFS>::graph_to_pairs(
                graph, { .cut_early_threshold = 8}, 4);

        assert(std::ranges::equal(pairs.sort_by_fwd().unique().fwd_pairs(),
                    typename decltype(pairs)::fwd_vec {
            { kmer_s("aca"), 6 },
            { kmer_s("aca"), 7 },
            { kmer_s("acg"), 6 },
            { kmer_s("acg"), 7 },
            { kmer_s("ata"), 6 },
            { kmer_s("ata"), 7 },
            { kmer_s("atg"), 6 },
            { kmer_s("atg"), 7 },
            { kmer_s("gca"), 6 },
            { kmer_s("gca"), 7 },
            { kmer_s("gcg"), 6 },
            { kmer_s("gcg"), 7 },
            { kmer_s("gta"), 6 },
            { kmer_s("gta"), 7 },
            { kmer_s("gtg"), 6 },
            { kmer_s("gtg"), 7 },
        }));

    });
});
