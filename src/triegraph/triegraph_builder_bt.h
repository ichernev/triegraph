#ifndef __TRIEGRAPH_BUILDER_BT_H__
#define __TRIEGRAPH_BUILDER_BT_H__

#include <chrono>
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

    decltype(pairs) &&get_pairs() && {
        auto time_01 = std::chrono::steady_clock::now();

        std::cerr << "BT builder" << std::endl;

        kmer = Kmer::empty();
        // this->data.trie_data.active_trie.insert(Kmer::empty());
        for (NodeLoc i = 0, nsz = graph.num_nodes(); i < nsz; ++i) {
            auto &node = graph.node(i);
            for (typename Str::Size nl = 0, sz = node.seg.size(); nl < sz; ++nl) {
                // kmer = Kmer::empty();
                assert(kmer.size() == 0);
                _back_track(NodePos(i, nl));
            }
        }
        auto time_02 = std::chrono::steady_clock::now();
        std::cerr << "Backtrack done: " << std::chrono::duration_cast<std::chrono::milliseconds>(time_02 - time_01).count() << "ms" << std::endl;

        // std::cerr << "num pairs: " << fast_pairs.size() << std::endl;
        // for (auto &x : fast_pairs) {
        //     std::cerr << x.first << " -> " << x.second << std::endl;
        // }

        // this->data.trie_data.init(std::move(fast_pairs), this->data.letter_loc);

        // auto time_03 = std::chrono::steady_clock::now();
        // std::cerr << "HT full: " << std::chrono::duration_cast<std::chrono::milliseconds>(time_03 - time_02).count() << "ms" << std::endl;
        // std::cerr << "total: " << std::chrono::duration_cast<std::chrono::milliseconds>(time_03 - time_01).count() << "ms" << std::endl;

        return std::move(pairs);
        // return std::move(data);
    }

    void _back_track(NodePos np) {
        if (this->kmer.is_complete()) {
            auto ll = this->lloc.compress(np);
            pairs.emplace_back(this->kmer, ll);
            // this->data.trie_data.trie2graph.add(this->kmer, ll);
            // this->data.trie_data.graph2trie.add(ll, this->kmer);
            // Kmer km = this->kmer;
            // while (true) {
            //     km.pop_back();
            //     if (this->data.trie_data.active_trie.contains(km))
            //         break;
            //     this->data.trie_data.active_trie.insert(km);
            // }
            return;
        }

        // this->data.trie_data.active_trie.insert(this->kmer);
        this->kmer.push_back(this->graph.node(np.node).seg[np.pos]);
        if (np.pos + 1 == this->graph.node(np.node).seg.size()) {
            for (const auto &fwd : this->graph.forward_from(np.node)) {
                _back_track(NodePos(fwd.node_id, 0));
            }
            // end-of-graph handling
            // if (this->data.graph.forward_from(np.node).empty() &&
            //         this->kmer.is_complete()) {
            //     _back_track(NodePos(this->data.letter_loc.num_locations, 0));
            // }
        } else {
            _back_track(NodePos(np.node, np.pos+1));
        }
        this->kmer.pop_back();
    }
};

} /* namespace triegraph */

#endif /* __TRIEGRAPH_BUILDER_BT_H__ */
