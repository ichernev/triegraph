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

    template <std::ranges::input_range R>
    decltype(pairs) &&get_pairs(const R &starts, u32 cut_early_threshold) && {
        auto scope = Logger::get().begin_scoped("pbfs builder");
        for (const auto &start: starts) {
            _bfs(start, cut_early_threshold);
        }

        return std::move(pairs);
    }

    std::vector<std::pair<Kmer, NodePos>> a, b;
    void _bfs(NodePos start, u32 cut_early_threshold) {

        if (auto &node = graph.node(start.node);
                start.pos + Kmer::K + 1 < node.seg.size()) {
            Kmer kmer;
            std::ranges::copy(
                    node.seg.get_view(start.pos, Kmer::K),
                    std::back_inserter(kmer));
            pairs.emplace_back(kmer, lloc.compress(
                        NodePos(start.node, start.pos+Kmer::K)));
            return;
        }

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
