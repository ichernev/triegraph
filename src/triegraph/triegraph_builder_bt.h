#ifndef __TRIEGRAPH_BUILDER_BT_H__
#define __TRIEGRAPH_BUILDER_BT_H__

#include "util/logger.h"

#include <chrono>
#include <ranges>
#include <assert.h>

namespace triegraph {

template <typename Graph_, typename LetterLocData_, typename Kmer_>
struct TrieGraphBTBuilder {
    using Graph = Graph_;
    using LetterLocData = LetterLocData_;
    using Kmer = Kmer_;
    using Str = Graph::Str;
    using NodeLoc = Graph::NodeLoc;
    using LetterLoc = LetterLocData::LetterLoc;
    using NodePos = LetterLocData::NodePos;

    const Graph &graph;
    const LetterLocData &lloc;
    std::vector<std::pair<Kmer, LetterLoc>> pairs;
    Kmer kmer;

    TrieGraphBTBuilder(const Graph &graph, const LetterLocData &lloc)
        : graph(graph), lloc(lloc) {}

    TrieGraphBTBuilder(const TrieGraphBTBuilder &) = delete;
    TrieGraphBTBuilder &operator= (const TrieGraphBTBuilder &) = delete;
    TrieGraphBTBuilder(TrieGraphBTBuilder &&) = delete;
    TrieGraphBTBuilder &operator= (TrieGraphBTBuilder &&) = delete;

    template <std::ranges::input_range R>
    decltype(pairs) &&get_pairs(const R &starts, u32 /* unused */) && {
        auto scope = Logger::get().begin_scoped("back track builder");
        kmer = Kmer::empty();
        for (const auto &start_np : starts) {
            assert(kmer.size() == 0);
            _back_track(start_np);
        }

        return std::move(pairs);
    }

    void _back_track(NodePos np) {
        if (this->kmer.is_complete()) {
            auto ll = this->lloc.compress(np);
            pairs.emplace_back(this->kmer, ll);
            return;
        }

        this->kmer.push_back(this->graph.node(np.node).seg[np.pos]);
        if (np.pos + 1 == this->graph.node(np.node).seg.size()) {
            for (const auto &fwd : this->graph.forward_from(np.node)) {
                _back_track(NodePos(fwd.node_id, 0));
            }
        } else {
            _back_track(NodePos(np.node, np.pos+1));
        }
        this->kmer.pop_back();
    }
};

} /* namespace triegraph */

#endif /* __TRIEGRAPH_BUILDER_BT_H__ */
