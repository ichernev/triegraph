#ifndef __TRIEGRAPH_BUILDER_PBFS_H__
#define __TRIEGRAPH_BUILDER_PBFS_H__

#include "util/logger.h"

#include <vector>
#include <ranges>

namespace triegraph {

template <typename Graph_, typename LetterLocData_, typename Kmer_>
struct TrieGraphBuilderPBFS {
    using Graph = Graph_;
    using LetterLocData = LetterLocData_;
    using Kmer = Kmer_;
    using NodePos = LetterLocData::NodePos;
    using LetterLoc = LetterLocData::LetterLoc;

    const Graph &graph;
    const LetterLocData &lloc;
    std::vector<std::pair<Kmer, LetterLoc>> pairs;

    TrieGraphBuilderPBFS(const Graph &graph, const LetterLocData &lloc)
        : graph(graph), lloc(lloc) {}

    TrieGraphBuilderPBFS(const TrieGraphBuilderPBFS &) = delete;
    TrieGraphBuilderPBFS(TrieGraphBuilderPBFS &&) = delete;
    TrieGraphBuilderPBFS &operator= (const TrieGraphBuilderPBFS &) = delete;
    TrieGraphBuilderPBFS &operator= (TrieGraphBuilderPBFS &&) = delete;

    decltype(pairs) &&get_pairs(std::ranges::input_range auto&& starts,
            u32 cut_early_threshold) && {
        auto scope = Logger::get().begin_scoped("pbfs builder");
        for (const auto &start: starts) {
            _bfs(start, cut_early_threshold);
        }
        Logger::get().log(
                "short", stats.short_kmer,
                "next", stats.short_next,
                "fast_split", stats.fast_split,
                "normal", stats.normal);
        return std::move(pairs);
    }

    struct Stats {
        u32 short_kmer;
        u32 short_next;
        u32 fast_split;
        u32 normal;
        Stats() : short_kmer(0), short_next(0), fast_split(0), normal(0) {}
    } stats;

    std::vector<std::pair<Kmer, NodePos>> a, b;
    void _bfs(NodePos start, u32 cut_early_threshold) {

        {
            auto &node = graph.node(start.node);
            if (start.pos + Kmer::K + 1 < node.seg.size()) {
                Kmer kmer;
                std::ranges::copy(
                        node.seg.get_view(start.pos, Kmer::K),
                        std::back_inserter(kmer));
                pairs.emplace_back(kmer, lloc.compress(
                            NodePos(start.node, start.pos+Kmer::K)));
                ++ stats.short_kmer;
                return;
            }
            auto left = node.seg.size() - start.pos;
            if (auto nxt = graph.forward_one(start.node);
                    nxt && Kmer::K - left < graph.node(*nxt).seg.size()) {
                Kmer kmer;
                std::ranges::copy(
                        node.seg.get_view(start.pos, left),
                        std::back_inserter(kmer));
                std::ranges::copy(
                        graph.node(*nxt).seg.get_view(0, Kmer::K - left),
                        std::back_inserter(kmer));
                pairs.emplace_back(kmer, lloc.compress(
                            NodePos(*nxt, Kmer::K - left)));
                ++ stats.short_next;
                return;
            }
            bool fast_split = true;
            for (const auto &fwd : graph.forward_from(start.node))
                if (Kmer::K - left >= fwd.seg.size())
                    fast_split = false;
            if (fast_split) {
                Kmer kmer;
                std::ranges::copy(
                        node.seg.get_view(start.pos, left),
                        std::back_inserter(kmer));
                for (const auto &fwd : graph.forward_from(start.node)) {
                    Kmer tmp = kmer;
                    std::ranges::copy(
                            fwd.seg.get_view(0, Kmer::K - left),
                            std::back_inserter(tmp));
                    pairs.emplace_back(tmp, lloc.compress(
                                NodePos(fwd.node_id, Kmer::K - left)));
                }
                ++ stats.fast_split;
                return;
            }
        }
        ++ stats.normal;

        // std::cerr << "running bfs" << std::endl;
        // a.reserve(cut_early_threshold);
        // b.reserve(cut_early_threshold);
        a.resize(0);

        std::vector<std::pair<Kmer, NodePos>> *crnt_q = &a, *next_q = &b;

        crnt_q->emplace_back(Kmer::empty(), start);
        typename Kmer::klen_type crnt_lvl;
        for (crnt_lvl = 0;
                crnt_lvl < Kmer::K &&
                    (cut_early_threshold == 0 || crnt_q->size() < cut_early_threshold);
                ++crnt_lvl) {
            next_q->resize(0);
            for (const auto &[kmer, np] : *crnt_q) {
                Kmer nkmer = kmer;
                nkmer.push_back(graph.node(np.node).seg[np.pos]);
                if (np.pos + 1 < graph.node(np.node).seg.size()) {
                    next_q->emplace_back(nkmer, NodePos(np.node, np.pos+1));
                } else {
                    for (const auto &fw : graph.forward_from(np.node)) {
                        next_q->emplace_back(nkmer, NodePos(fw.node_id, 0));
                    }
                }
            }
            std::swap(crnt_q, next_q);
        }

        // pairs.reserve(pairs.size() + crnt_q->size());
        auto conv_np = [&](const auto &p) { return std::make_pair(
                p.first, lloc.compress(p.second)); };
        std::ranges::copy(*crnt_q | std::ranges::views::transform(conv_np),
                std::back_inserter(pairs));
    }
};

} /* namespace triegraph */

#endif /* __TRIEGRAPH_BUILDER_PBFS_H__ */
