#ifndef __TRIE_BUILDER_LBFS_H__
#define __TRIE_BUILDER_LBFS_H__

#include "graph/connected_components.h"
#include "util/compressed_vector.h"
#include "util/util.h"
#include "util/logger.h"

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
#include <ranges>

namespace triegraph {

template <typename Graph_, typename LetterLocData_, typename Kmer_, typename VectorPairs_>
struct TrieBuilderLBFS {
    using Graph = Graph_;
    using LetterLocData = LetterLocData_;
    using Kmer = Kmer_;
    using VectorPairs = VectorPairs_;
    using Str = Graph::Str;
    using NodeLoc = Graph::NodeLoc;
    using LetterLoc = LetterLocData::LetterLoc;
    using NodePos = LetterLocData::NodePos;
    using kmer_len_type = u32; // how many kmers per location

    using Self = TrieBuilderLBFS;
    // using pairs_t = std::vector<std::pair<Kmer, LetterLoc>>;

    const Graph &graph;
    const LetterLocData &lloc;
    VectorPairs &pairs;

    struct Stats {
        u32 qpush;
        u32 qpop;
        u32 double_pop;
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
            : qpush(), qpop(), double_pop(), kpush(), kproc(), ksame(),
            tnodes(), maxk(), maxc(), maxid(), qsearch(), ssearch(),
            nsets() {
        }

        friend std::ostream &operator<< (std::ostream &os, const Stats &s) {
            return os << "----- STATS -----\n"
                << "qpush " << s.qpush << "\n"
                << "qpop " << s.qpop << "\n"
                << "double_pop " << s.double_pop << "\n"
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

    TrieBuilderLBFS(const Graph &graph, const LetterLocData &lloc, VectorPairs &pairs)
        : graph(graph), lloc(lloc), pairs(pairs) {
    }

    TrieBuilderLBFS(const Self &) = delete;
    TrieBuilderLBFS(Self &&) = delete;
    Self &operator= (const Self &) = delete;
    Self &operator= (Self &&) = delete;

    struct Settings {
        static constexpr u32 default_set_cutoff = 500u;
        u32 set_cutoff = default_set_cutoff;

        static Settings from_config(const auto &cfg) {
            return {
                .set_cutoff = cfg.template get_or<u32>(
                        "trie-builder-lbfs-set-cutoff", default_set_cutoff)
            };
        }
    } settings_;

    Self &set_settings(Settings &&s) { settings_ = std::move(s); return *this; }
    const Settings &settings() const { return settings_; }

    void compute_pairs(std::ranges::input_range auto&& /* starts */) {
        auto &log = Logger::get();
        auto scope = log.begin_scoped("bfs builder");

        log.begin("starting points");
        auto starts = ConnectedComponents<Graph>(graph).compute_starting_points();
        log.end().begin("bfs");
        auto kmer_data = _bfs_trie(std::move(starts));
        log.end().begin("converting to pairs");
        _fill_pairs(std::move(kmer_data));
    }

    struct KmerBuildData {
        std::vector<compressed_vector<Kmer>> kmers;
        std::vector<std::unordered_set<Kmer>> kmers_set;
        std::vector<kmer_len_type> done_idx;

        const u32 set_cutoff;
        Stats &stats;

        KmerBuildData(LetterLoc num, u32 set_cutoff, Stats &stats)
                : kmers(num+1),
                  kmers_set(num+1),
                  done_idx(num+1),
                  set_cutoff(set_cutoff),
                  stats(stats)
        {}

        bool exists(LetterLoc pos, Kmer kmer) const {
            auto &pkmers = kmers[pos];
            if (pkmers.size() >= set_cutoff) {
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

            if (pkmers.size() == set_cutoff) {
                ++stats.nsets;
                kmers_set[pos].insert(pkmers.begin(), pkmers.end());
            } else if (pkmers.size() > set_cutoff) {
                kmers_set[pos].insert(kmer);
            }
            return pkmers.size();
        }

        kmer_len_type num_kmers(LetterLoc pos) const {
            return kmers[pos].size();
        }
    };

    KmerBuildData _bfs_trie(const std::vector<NodeLoc> &starts) {
        // std::cerr << "==== bfs_trie" << std::endl;
        KmerBuildData kb(lloc.num_locations, settings_.set_cutoff, stats);

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

            if (kb.done_idx[h.node] > 0) {
                ++ stats.double_pop;
            }

            auto hc = lloc.compress(h);

            std::vector<NodePos> targets;
            if (graph.node(h.node).seg.size() == h.pos + 1) {
                for (const auto &to : graph.forward_from(h.node)) {
                    targets.push_back(NodePos(to.node_id, 0));
                }
            } else {
                assert(graph.node(h.node).seg.size() > h.pos + 1);
                targets.push_back(NodePos(h.node, h.pos+1));
            }

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
                    if (kb.exists(tc, kmer)) {
                        ++ stats.ksame;
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

    void _fill_pairs(KmerBuildData kb) {
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
        {
            auto scope = Logger::get().begin_scoped("freeing aux");
            auto _ = std::move(kb);
        }
    }
};

} /* namespace triegraph */

#endif /* __TRIE_BUILDER_LBFS_H__ */
