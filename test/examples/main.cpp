// SPDX-License-Identifier: MPL-2.0
/*
 * Copyright (c) 2021, Iskren Chernev
 */

#include "triegraph/dna_config.h"
#include "triegraph/manager.h"
#include "triegraph/util/logger.h"
#include "triegraph/util/cmdline.h"
#include "triegraph/util/human.h"

#include <assert.h>
#include <string>

using namespace std::literals;

using Logger = triegraph::Logger;

template <typename TG>
static TG::VectorPairs graph_to_pairs(
        const typename TG::Graph &graph,
        const typename TG::LetterLocData &lloc,
        typename TG::KmerSettings &&kmer_settings,
        const auto &cfg,
        typename TG::Algo algo) {
    switch (algo) {
        case TG::Algo::LOCATION_BFS:
            return TG::template graph_to_pairs<typename TG::TrieBuilderLBFS>(
                    graph, lloc, std::move(kmer_settings),
                    TG::TrieBuilderLBFS::Settings::from_config(cfg),
                    lloc);
        case TG::Algo::BACK_TRACK:
            return TG::template graph_to_pairs<typename TG::TrieBuilderBT>(
                    graph, lloc, std::move(kmer_settings),
                    TG::TrieBuilderBT::Settings::from_config(cfg),
                    lloc);
        case TG::Algo::POINT_BFS:
            return TG::template graph_to_pairs<typename TG::TrieBuilderPBFS>(
                    graph, lloc, std::move(kmer_settings),
                    TG::TrieBuilderPBFS::Settings::from_config(cfg),
                    lloc);
        case TG::Algo::NODE_BFS:
            return TG::template graph_to_pairs<typename TG::TrieBuilderNBFS>(
                    graph, lloc, std::move(kmer_settings),
                    TG::TrieBuilderNBFS::Settings::from_config(cfg),
                    lloc);
        default:
            throw "Unknown algorithm";
    }
}
// typename TG::vec_pairs get_pairs(const TG::Graph &graph, TG::Settings s,
//         decltype(TG::Settings::algo) algo) {
//     s.algo = algo;
//     if (algo == TG::Algo::BFS) {
//         return TG::pairs_from_graph<TG::TrieGraphBuilderBFS>(
//                 graph, s, TG::Settings::NoSkip {});
//     } else if (algo == TG::Algo::BACK_TRACK) {
//         return TG::pairs_from_graph<TG::TrieGraphBuilderBT>(
//                 graph, s, TG::Settings::NoSkip {});
//     } else if (algo == TG::Algo::POINT_BFS) {
//         return TG::pairs_from_graph<TG::TrieGraphBuilderPBFS>(
//                 graph, s, TG::Settings::NoSkip {});
//     } else if (algo == TG::Algo::NODE_BFS) {
//         return TG::pairs_from_graph<TG::TrieGraphBuilderNBFS>(
//                 graph, s, TG::Settings::NoSkip {});
//     }
//     assert(0);
//     return {};
// }

template <typename TG>
typename TG::TrieData get_td(
        const typename TG::Graph &graph,
        const typename TG::LetterLocData &lloc,
        auto &&cfg,
        typename TG::Algo algo) {
    auto ks = triegraph::KmerSettings::from_seed_config<typename TG::KmerHolder>(
            lloc.num_locations, cfg);
    auto pairs = graph_to_pairs<TG>(graph, lloc, std::move(ks), cfg, algo);
    pairs.sort_by_fwd().unique();
    // TG::prep_pairs(pairs);
    auto res = TG::pairs_to_triedata(std::move(pairs), lloc);

    auto stats = res.stats();
    std::cerr << stats << std::endl;
    std::cerr << "all kmers: " << TG::TrieData::total_kmers()
        << " used: " << double(stats.num_kmers) / TG::TrieData::total_kmers() << std::endl;
    std::cerr << "all locs: " << lloc.num_locations
        << " used: " << double(stats.num_locs) / lloc.num_locations << std::endl;

    return res;
}

template <typename TG>
void print_pairs(
        const typename TG::Graph &graph,
        const typename TG::LetterLocData &lloc,
        typename TG::VectorPairs pairs) {
    pairs.sort_by_fwd().unique();
    // TG::prep_pairs(pairs);

    auto st = Logger::get().begin_scoped("printing");
    for (const auto &p : pairs.fwd_pairs()) {
        typename TG::NodePos np = lloc.expand(p.second);
        const auto &node = graph.node(np.node);
        std::cout << p << " " << np.node << "(" << node.seg_id << "):"
            << np.pos << "/" << node.seg.size() << std::endl;
    }
}

template <typename TG>
std::vector<typename TG::NodeLoc> shortest_path(
        const typename TG::Graph &graph,
        typename TG::NodeLoc start,
        typename TG::NodeLoc finish) {
    // node, previous-id
    std::vector<std::pair<typename TG::NodeLoc, typename TG::NodeLoc>> q;
    std::unordered_set<typename TG::NodeLoc> vis;

    q.emplace_back(start, TG::Graph::INV_SIZE);
    vis.emplace(start);
    typename TG::NodeLoc qp;
    for (qp = 0; qp < q.size(); ++qp) {
        auto [node, prev_id] = q[qp];

        if (node == finish)
            break;

        for (const auto &fwd : graph.forward_from(node)) {
            if (!vis.contains(fwd.node_id)) {
                q.emplace_back(fwd.node_id, qp);
                vis.emplace(fwd.node_id);
            }
        }
    }

    if (qp == q.size())
        return {};

    // restore path
    std::vector<typename TG::NodeLoc> res;
    while (qp != TG::Graph::INV_SIZE) {
        // std::cerr << "qp " << qp << std::endl;
        res.emplace_back(q[qp].first);
        qp = q[qp].second;
    }
    std::ranges::reverse(res);
    return res;
}

int main(int argc, char *argv[]) {
    auto cmdline = triegraph::CmdLine(argc, argv);

    assert(cmdline.positional.size() == 3);
    auto cmd = cmdline.positional[0];
    auto graph_file = cmdline.positional[1];
    auto algo = cmdline.positional[2];
    // auto td_rel = cmdline.get_or<triegraph::i32>("trie-depth-rel", 0);
    // auto td_abs = cmdline.get_or<triegraph::u32>("trie-depth", 0);

    try {
        if (cmd == "pairs"s || cmd == "print-pairs"s ||
                cmd == "td"s || cmd == "td0"s || cmd == "td-cv"s ||
                cmd == "ce-test"s ||
                cmd == "print-top-order"s) {

            using triegraph::dna::CfgFlags;
            using TG = triegraph::Manager<triegraph::dna::DnaConfig<0,
                  CfgFlags::VP_RAW_KMERS>>;
            auto algo_v = TG::algo_from_name(algo);
            assert(algo_v != TG::Algo::UNKNOWN);

            auto graph = TG::Graph::from_file(graph_file, {});
            auto lloc = TG::LetterLocData(graph);
            // TG::Settings s {
            //     .trie_depth = (td_abs == 0 ?
            //         triegraph::log4_ceil(lloc.num_locations) + td_rel :
            //         td_abs)
            // };
            // TG::init(s);

            if (cmd == "pairs"s || cmd == "print-pairs"s ||
                    cmd == "ce-test"s) {
                // auto pairs = get_pairs(graph, s, algo_v);
                auto pairs = graph_to_pairs<TG>(graph, lloc,
                        triegraph::KmerSettings::from_seed_config<TG::KmerHolder>(lloc.num_locations, cmdline),
                        cmdline, algo_v);

                if (cmd == "print-pairs"s)
                    print_pairs<TG>(graph, lloc, std::move(pairs));
                else if (cmd == "ce-test"s) {
                    pairs.sort_by_rev().unique();
                    // TG::prep_pairs(pairs);
                    // std::ranges::sort(pairs, [](const auto &a, const auto &b) {
                    //         return a.second < b.second;
                    // });
                    auto ce = TG::ComplexityEstimator(
                            graph,
                            TG::TopOrder::Builder(graph).build(),
                            TG::Kmer::K,
                            4, // backedge_init
                            2).compute(); // backedge_max_trav

                    // for (TG::NodeLoc i = 0; i < graph.num_nodes(); ++i) {
                    //     std::cout << graph.node(i).seg_id << " "
                    //         << ce.get_starts()[i] << std::endl;
                    //     // auto np_beg = lloc.compress(TG::NodePos(i, 0));
                    //     // auto er_beg = std::ranges::equal_range(
                    //     //         pairs, std::make_pair(TG::Kmer::empty(), np_beg),
                    //     //         [](const auto &a, const auto &b) {
                    //     //             return a.second < b.second;
                    //     //         });
                    //     // std::cout << graph.node(i).seg_id << " "
                    //     //     << er_beg.size() << std::endl;
                    // }

                    for (TG::NodeLoc i = 0; i < graph.num_nodes(); ++i) {
                        auto np_beg = lloc.compress(TG::NodePos(i, 0));
                        auto er_beg = std::ranges::equal_range(
                                pairs.fwd_pairs(),
                                std::make_pair(TG::Kmer::empty(), np_beg),
                                [](const auto &a, const auto &b) {
                                    return a.second < b.second;
                                });
                        if (ce.get_starts()[i] < er_beg.size()) {
                            std::cerr << "BEG issue at node " << i << " "
                                << graph.node(i).seg_id << std::endl;
                            std::cerr << ce.get_starts()[i] << " " << er_beg.size() << std::endl;
                            assert(ce.get_starts()[i] >= er_beg.size());
                        }
                    }
                }
            } else if (cmd == "td"s) {
                using TGX = triegraph::Manager<triegraph::dna::DnaConfig<0, CfgFlags::TD_SORTED_VECTOR | CfgFlags::VP_DUAL_IMPL>>;
                auto td = get_td<TGX>(graph, lloc, cmdline, TGX::algo_from_name(algo));
            } else if (cmd == "td0"s) {
                using TGX = triegraph::Manager<triegraph::dna::DnaConfig<0, CfgFlags::VP_DUAL_IMPL | CfgFlags::TD_SORTED_VECTOR | CfgFlags::TD_ZERO_OVERHEAD>>;
                auto td = get_td<TGX>(graph, lloc, cmdline, TGX::algo_from_name(algo));
            } else if (cmd == "td-cv"s) {
                using TG_CV = triegraph::Manager<triegraph::dna::DnaConfig<0, CfgFlags::VP_DUAL_IMPL | CfgFlags::TD_SORTED_VECTOR | CfgFlags::CV_ELEMS>>;
                auto td = get_td<TG_CV>(graph, lloc, cmdline, TG_CV::algo_from_name(algo));
            } else if (cmd == "print-top-order"s) {
                // auto starts = ConnectedComponents<TG::Graph>(graph).compute_starting_points();
                // auto top_ord = TG::TopOrder::Builder(graph).build(starts);
                auto top_ord = TG::TopOrder::Builder(graph).build();
                for (TG::NodeLoc i = 0; i < graph.num_nodes(); ++i) {
                    std::cout << "N " << graph.node(i).seg_id << " " << top_ord.idx[i] << std::endl;
                }
                for (const auto &edge : graph.forward_edges()) {
                    if (top_ord.is_backedge(edge)) {
                        std::cout << "E " << graph.node(edge.from).seg_id << " " << graph.node(edge.to).seg_id
                            << " EST:" << top_ord.idx[edge.to] - top_ord.idx[edge.from]
                            << std::endl;
                        auto sp = shortest_path<TG>(graph, edge.to, edge.from);
                        std::cout << " shortest len: " << sp.size() << std::endl;
                        if (sp.size() < 50) {
                            for (const auto node_id : sp) {
                                std::cout << " " << graph.node(node_id).seg_id;
                            }
                            std::cout << std::endl;
                        }
                    }
                }
            }
        } else if (cmd == "graph-stat") {
            using triegraph::to_human_number;
            using TG = triegraph::Manager<triegraph::dna::DnaConfig<0>>;
            // auto algo_v = TG::algo_from_name(algo);
            // assert(algo_v != TG::Algo::UNKNOWN);

            auto graph = TG::Graph::from_file(graph_file, {});
            auto lloc = TG::LetterLocData(graph);

            std::cerr << "num nodes: " << to_human_number(graph.num_nodes(), false) << '\n'
                << "num edges: " << to_human_number(graph.num_edges(), false) << '\n'
                << "num locs: " << to_human_number(lloc.num_locations, false) << '\n'
                << std::flush;
        } else {
            for (int i = 0; i < argc; ++i) {
                std::cerr << i << " '" <<  argv[i] << "'" << std::endl;
            }
        }
    } catch (const char *e) {
        std::cerr << "got exception " << e << std::endl;
    }

    return 0;
}
