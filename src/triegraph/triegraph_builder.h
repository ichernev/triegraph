#ifndef __TRIEGRAPH_BUILDER_H__
#define __TRIEGRAPH_BUILDER_H__

#include "graph/connected_components.h"
#include "util/compressed_vector.h"
#include "util/util.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_set>
#include <vector>
#include <string>
#include <type_traits>
#include <queue>
#include <limits>
#include <cassert>
#include <chrono>

namespace triegraph {

template <typename Graph_, typename LetterLocData_, typename Kmer_>
struct TrieGraphBuilder {
    using Graph = Graph_;
    using LetterLocData = LetterLocData_;
    using Kmer = Kmer_;
    using Str = Graph::Str;
    using NodeLoc = Graph::NodeLoc;
    using LetterLoc = LetterLocData::LetterLoc;
    using NodePos = LetterLocData::NodePos;
    using kmer_len_type = u32; // how many kmers per location

    const Graph &graph;
    const LetterLocData &lloc;

    struct Stats {
        u32 qpush;
        u32 qpop;
        u32 kpush;
        u32 kproc;
        u32 ksame;
        u32 tnodes;
        u32 maxk;
        u32 maxc;
        u32 maxid;
        u32 qsearch;
        u32 ssearch;
        u32 nsets;

        Stats()
            : qpush(), qpop(), kpush(), kproc(), ksame(),
            tnodes(), maxk(), maxc(), maxid(), qsearch(), ssearch(),
            nsets() {
        }

        friend std::ostream &operator<< (std::ostream &os, const Stats &s) {
            return os << "----- STATS -----\n"
                << "qpush " << s.qpush << "\n"
                << "qpop " << s.qpop << "\n"
                << "kpush " << s.kpush << "\n"
                << "kproc " << s.kproc << "\n"
                << "ksame " << s.ksame << "\n"
                << "tnodes " << s.tnodes << "\n"
                << "maxk " << s.maxk << "\n"
                << "maxk " << s.maxc << "\n"
                << "maxid " << s.maxid << "\n"
                << "qsearch " << s.qsearch << "\n"
                << "ssearch " << s.ssearch << "\n"
                << "nsets " << s.nsets;
        }
    } stats;

    TrieGraphBuilder(const Graph &graph, const LetterLocData &lloc)
        : graph(graph), lloc(lloc) {
    }

    TrieGraphBuilder(const TrieGraphBuilder &) = delete;
    TrieGraphBuilder &operator= (const TrieGraphBuilder &) = delete;
    TrieGraphBuilder(TrieGraphBuilder &&) = delete;
    TrieGraphBuilder &operator= (TrieGraphBuilder &&) = delete;

    std::vector<std::pair<Kmer, LetterLoc>> get_pairs() {
        auto time_01 = std::chrono::steady_clock::now();

        std::cerr << "=== building trie === " << std::endl;
        auto starts = ConnectedComponents<Graph>(graph).compute_starting_points();
        // auto starts = _compute_starts(std::move(comps));
        auto kmer_data = _bfs_trie(std::move(starts));
        // _fill_trie_data(std::move(kmer_data));
        auto time_02 = std::chrono::steady_clock::now();
        std::cerr << "BFS done: " << std::chrono::duration_cast<std::chrono::milliseconds>(time_02 - time_01).count() << "ms" << std::endl;
        // _print_stats(std::move(kmer_data));
        return _make_pairs(std::move(kmer_data));
        // _fill_trie_data_opt(std::move(kmer_data));
        // auto time_03 = std::chrono::steady_clock::now();
        // std::cerr << "trie filled: " << std::chrono::duration_cast<std::chrono::milliseconds>(time_03 - time_02).count() << "ms" << std::endl;
        // // _print_stats(std::move(kmer_data));

        // return std::move(data);
        // return Triegraph {
        //     std::move(graph),
        //     std::move(lloc),
        //     std::move(trie_data)
        // };
    }

    struct KmerBuildData {
        static constexpr u32 SET_CUTOFF = 500;
        std::vector<compressed_vector<Kmer>> kmers;
        // std::vector<std::vector<Kmer>> kmers;
        std::vector<std::unordered_set<Kmer>> kmers_set;

        std::vector<kmer_len_type> done_idx;

        Stats &stats;

        KmerBuildData(LetterLoc num, Stats &stats) : stats(stats) {
            // one more for fictional position to store kmers that match to end
            kmers.resize(num + 1);
            kmers_set.resize(num + 1);
            done_idx.resize(num + 1);
        }

        bool exists(LetterLoc pos, Kmer kmer) const {
            auto &pkmers = kmers[pos];
            if (pkmers.size() >= SET_CUTOFF) {
                ++stats.qsearch;
                return kmers_set[pos].contains(kmer);
            } else {
                ++stats.ssearch;
                return std::find(pkmers.begin(), pkmers.end(), kmer) != pkmers.end();
            }
        }

        kmer_len_type add_kmer(LetterLoc pos, Kmer kmer) {
            auto &pkmers = kmers[pos];
            pkmers.emplace_back(kmer);

            if (pkmers.size() == SET_CUTOFF) {
                ++stats.nsets;
                kmers_set[pos].insert(pkmers.begin(), pkmers.end());
            } else if (pkmers.size() > SET_CUTOFF) {
                kmers_set[pos].insert(kmer);
            }
            return pkmers.size();
        }

        kmer_len_type num_kmers(LetterLoc pos) const {
            return kmers[pos].size();
        }
    };

    KmerBuildData _bfs_trie(const std::vector<NodeLoc> &starts) {
        std::cerr << "==== bfs_trie" << std::endl;
        KmerBuildData kb(lloc.num_locations, stats);

        std::queue<NodePos> q;

        for (auto start : starts) {
            auto h = NodePos(start, 0);
            ++ stats.qpush;
            ++ stats.kpush;
            ++ stats.tnodes;
            q.push(h);
            kb.kmers[lloc.compress(h)].push_back(Kmer::empty());
        }

        bool verbose_mode = false;
        while (!q.empty()) {
            auto h = q.front(); q.pop();
            ++ stats.qpop;

            // if (stats.qpop > 788300) {
            //     verbose_mode = true;
            // }
            // if (stats.qpop % 100000 == 0 || verbose_mode) {
            //     std::cerr << stats << std::endl;
            //     std::cerr << "current location "
            //         << h.node << " " << h.pos << std::endl;
            //     auto z = lloc.expand(stats.maxid);
            //     std::cerr << stats.maxid << " " << z.node << " " << z.pos << std::endl;
            //     std::cerr << graph.nodes[z.node].seg_id << std::endl;
            // }

            auto hc = lloc.compress(h);
            // if (hc == lloc.num_locations) {
            //     // this position signifies kmers that match to the end of the
            //     // graph
            //     stats.kproc += kb.kmers[hc].size() - kb.done_idx[hc];
            //     kb.done_idx[hc] = kb.kmers[hc].size();
            //     continue;
            // }

            std::vector<NodePos> targets;
            if (graph.node(h.node).seg.size() == h.pos + 1) {
                for (const auto &to : graph.forward_from(h.node)) {
                    targets.push_back(NodePos(to.node_id, 0));
                }
            } else {
                assert(graph.node(h.node).seg.size() > h.pos + 1);
                targets.push_back(NodePos(h.node, h.pos+1));
            }

            // if (targets.empty()) {
            //     // This signifies matching the end.
            //     // Needs support from LetterLocData::compress
            //     targets.push_back(NodePos(letter_loc.num_locations, 0));
            // }
            if (verbose_mode)
                std::cerr << "ksz " << kb.kmers[hc].size() << "\n"
                        << "ktd " << kb.done_idx[hc] << "\n"
                        << "tgts " << targets.size() << std::endl;

            auto &done_idx = kb.done_idx[hc];
            auto &kmers = kb.kmers[hc];
            auto letter = graph.node(h.node).seg[h.pos];
            for (; done_idx < kmers.size(); ++done_idx) {
                auto kmer = kmers[done_idx];
                ++ stats.kproc;
                kmer.push(letter);
                for (auto t: targets) {
                    auto tc = lloc.compress(t);
                    // auto &t_kmers = kb.kmers[tc];
                    // if (verbose_mode)
                    // std::cerr << "   will check " << t_kmers.size() << " for match" << std::endl;
                    if (kb.exists(tc, kmer)) {
                    // if (std::find(t_kmers.begin(), t_kmers.end(), kmer) != t_kmers.end()) {
                        ++ stats.ksame;
                        // do not add kmer twice
                        continue;
                    }
                    auto t_done = kb.done_idx[tc];
                    ++ stats.kpush;
                    auto nk = kb.add_kmer(tc, kmer);
                    // t_kmers.emplace_back(kmer);
                    if (nk == 1) {
                        ++ stats.tnodes;
                    }
                    if (nk > stats.maxk) {
                        stats.maxk = nk;
                        stats.maxc = 1;
                        stats.maxid = tc;
                    } else if (nk == stats.maxk) {
                        ++ stats.maxc;
                    }
                    if (t_done + 1u == nk) {
                        ++ stats.qpush;
                        q.push(t);
                    }
                }
            }
        }
        return kb;
    }

    void _print_stats(KmerBuildData kb) {
        std::cerr << "printing stats" << std::endl;
        int buckets[31] = {0,};
        long long total = 0;
        long long fast = 0;
        for (LetterLoc i = 0; i <= lloc.num_locations; ++i) {
            buckets[log2_ceil(kb.kmers[i].size())] += 1;
            total += kb.kmers[i].size();
            fast += kb.kmers[i].is_fast();
        }
        std::cerr << "fast " << fast << " " << lloc.num_locations << std::endl;
        long long ctot = 0;
        for (int i = 0; i < 31; ++i) {
            ctot += buckets[i];
            std::cerr << i << "\t" << buckets[i] << " " << (double)ctot / lloc.num_locations << std::endl;
        }
        std::cerr << "total " << total << std::endl;
        std::cerr << "num locations " << lloc.num_locations << std::endl;
    }

    // void _fill_trie_data(KmerBuildData kb) {
    //     std::cerr << "==== fill trie data" << std::endl;
    //     u32 added1 = 0, added2 = 0;
    //     auto &trie_data = data.trie_data;
    //     for (LetterLoc i = 0; i <= lloc.num_locations; ++i) {
    //         if (kb.kmers[i].size() > std::numeric_limits<typename decltype(kb.done_idx)::value_type>::max()) {
    //             std::cerr << "got " << kb.kmers[i].size() << " kmers at " << i << std::endl;
    //             throw "too-many-kmers";
    //         }
    //         for (kmer_len_type j = 0; j < kb.done_idx[i]; ++j) {
    //             auto kmer = kb.kmers[i][j];
    //             if (kmer.is_complete()) {
    //                 // std::cerr << "full kmer " << kb.kmers[i][j] << " " << i << std::endl;
    //                 ++added1;
    //                 if (added1 % 100000 == 0) {
    //                     std::cerr << "Added1 " << added1 << "\n"
    //                         << i << "/" << lloc.num_locations
    //                         << std::endl;
    //                 }
    //                 trie_data.trie2graph.add(kb.kmers[i][j], i);
    //             }
    //         }
    //     }

    //     std::cerr << "destroying kb" << std::endl;
    //     // destroy kb
    //     { auto _ = std::move(kb); }
    //     std::cerr << "destroyed kb" << std::endl;

    //     // put a guard for kmer pop loop
    //     trie_data.active_trie.emplace(Kmer::empty());
    //     for (const auto &item : trie_data.trie2graph) {
    //         ++added2;
    //         if (added2 % 100000 == 0) {
    //             std::cerr << "Added2 " << added2 << std::endl;
    //         }
    //         trie_data.graph2trie.add(item.second, item.first);
    //         auto kmer = item.first;
    //         do {
    //             kmer.pop();
    //         } while (trie_data.active_trie.insert(kmer).second);
    //     }
    // }

    std::vector<std::pair<Kmer, LetterLoc>> _make_pairs(KmerBuildData kb) {
        std::cerr << "prepping pairs" << std::endl;
        std::vector<std::pair<Kmer, LetterLoc>> pairs;
        u64 total = 0;
        assert(kb.kmers.size() == 1 + lloc.num_locations);
        for (LetterLoc i = 0; i <= lloc.num_locations; ++i) {
            total += kb.kmers[i].size();
        }
        pairs.reserve(total);
        for (LetterLoc i = 0; i <= lloc.num_locations; ++i) {
            for (const auto &kmer : kb.kmers[i]) {
                if (kmer.is_complete()) {
                    pairs.emplace_back(kmer, i);
                }
            }
        }

        // destroy kb
        { auto _ = std::move(kb); }

        return pairs;

        // std::cerr << "pairs ready" << std::endl;
        // data.trie_data.init(std::move(pairs), data.letter_loc);
    }
};

} /* namespace triegraph */

#endif /* __TRIEGRAPH_BUILDER_H__ */
