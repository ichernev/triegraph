// SPDX-License-Identifier: MPL-2.0
/*
 * Copyright (c) 2021, Iskren Chernev
 */

#include "triegraph/dna_config.h"
#include "triegraph/manager.h"

#include "triegraph/util/cmdline.h"
#include "triegraph/util/logger.h"

using namespace triegraph;

// struct Cfg : dna::DnaConfig<0> {
//     static constexpr bool triedata_allow_inner = true;
// };

using triegraph::dna::CfgFlags;
using TG = Manager<dna::DnaConfig<0,
      CfgFlags::ALLOW_INNER_KMER |
      CfgFlags::VP_DUAL_IMPL |
      CfgFlags::TD_SORTED_VECTOR>>;

// template <typename Builder, std::ranges::input_range R>
// static TG::vec_pairs get_pairs_impl(
//         const TG::Graph &graph,
//         const TG::LetterLocData &lloc,
//         R&& cc_seed,
//         TG::Settings s) {
//     return Builder(graph, lloc).get_pairs(std::forward<R>(cc_seed), s.cut_early_threshold);
// }

static TG::VectorPairs get_pairs(
        const TG::Graph &graph,
        const TG::LetterLocData &lloc,
        const TG::Algo algo,
        std::ranges::input_range auto&& cc_seed,
        auto&& cfg) {
    switch (algo) {
        case TG::Algo::BACK_TRACK:
            return TG::graph_to_pairs<TG::TrieBuilderBT>(
                    graph, lloc, TG::Kmer::settings(),
                    TG::TrieBuilderBT::Settings::from_config(cfg),
                    std::forward<decltype(cc_seed)>(cc_seed));
        case TG::Algo::POINT_BFS:
            return TG::graph_to_pairs<TG::TrieBuilderPBFS>(
                    graph, lloc, TG::Kmer::settings(),
                    TG::TrieBuilderPBFS::Settings::from_config(cfg),
                    std::forward<decltype(cc_seed)>(cc_seed));
        default:
            throw "Unsupported algorithm. Algorithm should support starts (BT, PBFS)";
            return {};
    }
}

int main(int argc, char *argv[]) {
    using std::string_literals::operator""s;

    auto cmdline = CmdLine(argc, argv);
    cmdline.debug();

    auto algo_fast = cmdline.get_or<std::string>("algo-fast", "back_track"s);
    auto algo_slow = cmdline.get_or<std::string>("algo-slow", "pbfs"s);
    auto backedge_init = cmdline.get_or<u32>("cc-backedge-init", 4u);
    auto backedge_max_trav = cmdline.get_or<u32>("cc-backedge-max-trav", 2u);
    auto cc_cutoff = cmdline.get_or<u32>("cc-cutoff", 512u);
    // auto pbfs_kmer_cutoff = cmdline.get_or<u32>("pbfs-kmer-cutoff", 256u);
    auto gfa_file = cmdline.get<std::string>("graph");
    // auto td_rel = cmdline.get_or<i32>("trie-depth-rel", 0);
    // auto td_abs = cmdline.get_or<u32>("trie-depth", 0);

    Logger::get().begin("reading graph");
    auto graph = TG::Graph::from_file(gfa_file, {});
    Logger::get().end().begin("building lloc");
    auto lloc = TG::LetterLocData(graph);
    // TG::Settings s {
    //     .trie_depth = td_abs == 0 ?
    //         triegraph::log4_ceil(lloc.num_locations) + td_rel :
    //         td_abs,
    //     .cut_early_threshold = pbfs_kmer_cutoff,
    // };
    Logger::get().end().begin("TG::Kmer::set_settings");
    TG::Kmer::set_settings(
            TG::KmerSettings::from_seed_config(lloc.num_locations, cmdline));
    // TG::init(s);

    Logger::get().end().begin("top_order build");
    auto top_ord = TG::TopOrder::Builder(graph).build();
    Logger::get().end().begin("CE building");
    auto ce = TG::ComplexityEstimator(
            graph,
            top_ord,
            TG::Kmer::K,
            backedge_init,
            backedge_max_trav)
        .compute();

    auto cc_seed_v = std::views::iota(0u)
        | std::views::take(graph.num_nodes())
        | std::views::filter([&ce, cc_cutoff] (auto n_id) {
                // std::cerr << "-WWX " << n_id << std::endl;
                assert(n_id < ce.get_ends().size());
                return ce.get_ends()[n_id] >= cc_cutoff; });

    // std::cerr << "X_____X" << std::endl;
    // std::ranges::copy(cc_seed_v, std::ostream_iterator<TG::NodeLoc>(std::cerr, "\n"));

    Logger::get().end().begin("CCW building");
    auto ccw = TG::ComplexityComponentWalker::Builder(
            graph,
            TG::Kmer::K)
        .build(cc_seed_v);

    auto num_cc_starts = std::ranges::distance(ccw.cc_starts(graph, TG::Kmer::K));
    auto num_non_cc_starts = std::ranges::distance(ccw.non_cc_starts(graph, TG::Kmer::K));
    auto num_all_starts = lloc.num_locations;

    Logger::get().log(
            "cc_starts", num_cc_starts,
            "as %", (num_cc_starts * 100.0) / num_all_starts,
            "non_cc_starts", num_non_cc_starts,
            "as %", (num_non_cc_starts * 100.0) / num_all_starts);

    Logger::get().end().begin("CCW non_cc_starts");
    auto p1 = get_pairs(graph, lloc, TG::algo_from_name(algo_fast),
            ccw.non_cc_starts(graph, TG::Kmer::K), cmdline);

    Logger::get().end().begin("CCW cc_starts");
    auto p2 = get_pairs(graph, lloc, TG::algo_from_name(algo_slow),
            ccw.cc_starts(graph, TG::Kmer::K), cmdline);

    Logger::get().end().begin("merge pairs");
    std::ranges::copy(p2.fwd_pairs(), std::back_inserter(p1));
    { auto _ = std::move(p2); }

    p1.sort_by_fwd().unique();

    Logger::get().end().begin("build TD");
    auto td = TG::TrieData(std::move(p1), lloc);
    Logger::get().end();

    // std::cerr << td.stats() << std::endl;

    return 0;
}
