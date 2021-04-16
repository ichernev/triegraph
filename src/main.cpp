#include "dna_config.h"
#include "manager.h"
#include "util/logger.h"
#include "util/cmdline.h"

#include <assert.h>
#include <string>

using namespace std::literals;

using TG = triegraph::Manager<triegraph::dna::DnaConfig<0>>;
using Logger = triegraph::Logger;

typename TG::vec_pairs get_pairs(const TG::Graph &graph, TG::Settings s,
        decltype(TG::Settings::algo) algo) {
    s.algo = algo;
    if (algo == TG::Settings::BFS) {
        return TG::pairs_from_graph<TG::TrieGraphBuilderBFS>(
                graph, s, TG::Settings::NoSkip {});
    } else if (algo == TG::Settings::BACK_TRACK) {
        return TG::pairs_from_graph<TG::TrieGraphBuilderBT>(
                graph, s, TG::Settings::NoSkip {});
    } else if (algo == TG::Settings::POINT_BFS) {
        return TG::pairs_from_graph<TG::TrieGraphBuilderPBFS>(
                graph, s, TG::Settings::NoSkip {});
    } else if (algo == TG::Settings::NODE_BFS) {
        return TG::pairs_from_graph<TG::TrieGraphBuilderNBFS>(
                graph, s, TG::Settings::NoSkip {});
    }
    assert(0);
    return {};
}

typename TG::TrieData get_td(const TG::Graph &graph, const TG::LetterLocData &lloc,
        TG::Settings s, decltype(TG::Settings::algo) algo, bool do_sanity_check) {
    auto pairs = get_pairs(graph, s, algo);
    auto res = TG::TrieData(pairs, lloc);

    if (do_sanity_check) {
        auto scope = Logger::get().begin_scoped("sanity check");
        res.sanity_check(pairs, lloc);
    }

    std::cerr << "T2G Histogrm:" << std::endl;
    res.t2g_histogram().print(std::cerr);
    std::cerr << "G2T Histogrm:" << std::endl;
    res.g2t_histogram().print(std::cerr);
    auto nkmers = std::ranges::distance(res.trie2graph.keys());
    auto nlocs = std::ranges::distance(res.graph2trie.keys());
    std::cerr << "num kmers: " << nkmers << std::endl;
    std::cerr << "all kmers: " << TG::TrieData::total_kmers() << std::endl;
    std::cerr << "num locs: " << nlocs << std::endl;
    std::cerr << "all locs: " << lloc.num_locations << std::endl;
    std::cerr << "ff: " << double(nkmers) / nlocs << std::endl;
    std::cerr << "used kmers: " << double(nkmers) / TG::TrieData::total_kmers() << std::endl;

    return res;
}

void prep_pairs(TG::vec_pairs &pairs) {
    {
        auto st = Logger::get().begin_scoped("sorting pairs");
        std::ranges::sort(pairs);
    }
    {
        auto st = Logger::get().begin_scoped("removing dupes");
        auto sr = std::ranges::unique(pairs);
        auto new_size = sr.begin() - pairs.begin();
        pairs.resize(new_size);
    }
}

void print_pairs(const TG::Graph &graph, const TG::LetterLocData &lloc,
        TG::vec_pairs pairs) {
    prep_pairs(pairs);

    auto st = Logger::get().begin_scoped("printing");
    for (const auto &p : pairs) {
        TG::NodePos np = lloc.expand(p.second);
        const auto &node = graph.node(np.node);
        std::cout << p << " " << np.node << "(" << node.seg_id << "):"
            << np.pos << "/" << node.seg.size() << std::endl;
    }
}

std::vector<TG::NodeLoc> shortest_path(const TG::Graph &graph,
        TG::NodeLoc start, TG::NodeLoc finish) {
    // node, previous-id
    std::vector<std::pair<TG::NodeLoc, TG::NodeLoc>> q;
    std::unordered_set<TG::NodeLoc> vis;

    q.emplace_back(start, TG::Graph::INV_SIZE);
    vis.emplace(start);
    TG::NodeLoc qp;
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
    std::vector<TG::NodeLoc> res;
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
    auto gfa_file = cmdline.positional[1];
    auto algo = cmdline.positional[2];
    auto td_rel = cmdline.get_or<triegraph::i32>("trie-depth-rel", 0);
    auto td_abs = cmdline.get_or<triegraph::u32>("trie-depth", 0);

    try {
        if (cmd == "pairs"s || cmd == "print-pairs"s ||
                cmd == "td"s || cmd == "ce-test"s ||
                cmd == "print-top-order"s) {

            assert(algo == "bfs" || algo == "back_track" || algo == "pbfs" ||
                    algo == "node_bfs");

            auto algo_v = algo == "bfs" ? TG::Settings::BFS :
                algo == "back_track" ? TG::Settings::BACK_TRACK :
                algo == "pbfs" ? TG::Settings::POINT_BFS :
                TG::Settings::NODE_BFS;
            auto graph = TG::Graph::from_file(gfa_file, {});
            auto lloc = TG::LetterLocData(graph);
            TG::Settings s {
                .trie_depth = (td_abs == 0 ?
                    triegraph::log4_ceil(lloc.num_locations) + td_rel :
                    td_abs)
            };
            // TG::init(s);

            if (cmd == "pairs"s || cmd == "print-pairs"s ||
                    cmd == "ce-test"s) {
                auto pairs = get_pairs(graph, s, algo_v);

                if (cmd == "print-pairs"s)
                    print_pairs(graph, lloc, std::move(pairs));
                else if (cmd == "ce-test"s) {
                    prep_pairs(pairs);
                    std::ranges::sort(pairs, [](const auto &a, const auto &b) {
                            return a.second < b.second;
                    });
                    auto ce = TG::ComplexityEstimator(
                            graph,
                            TG::TopOrder::Builder(graph).build(),
                            s.trie_depth,
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
                                pairs, std::make_pair(TG::Kmer::empty(), np_beg),
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
                auto td = get_td(graph, lloc, s, algo_v, cmdline.get<bool>("--sanity-check"));
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
                        auto sp = shortest_path(graph, edge.to, edge.from);
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
