#ifndef __TRIEGRAPH_BUILDER_BT_H__
#define __TRIEGRAPH_BUILDER_BT_H__

#include <chrono>
#include <assert.h>

namespace triegraph {

template <typename TrieGraphData_>
struct TrieGraphBTBuilder {
    using TrieGraphData = TrieGraphData_;
    using Graph = TrieGraphData::Graph;
    using LetterLocData = TrieGraphData::LetterLocData;
    using TrieData = TrieGraphData::TrieData;
    using Str = Graph::Str;
    using NodeLoc = Graph::NodeLoc;
    using LetterLoc = LetterLocData::LetterLoc;
    using NodePos = LetterLocData::NodePos;
    using Kmer = TrieData::Kmer;
    using kmer_len_type = u32; // u16; // assume no more than 64k kmers end in a given letter location

    TrieGraphData data;
    Kmer kmer;
    std::vector<std::pair<Kmer, LetterLoc>> fast_pairs;

    TrieGraphBTBuilder(Graph &&graph) : data(std::move(graph)), kmer(Kmer::empty()) {}

    TrieGraphBTBuilder(const TrieGraphBTBuilder &) = delete;
    TrieGraphBTBuilder &operator= (const TrieGraphBTBuilder &) = delete;
    TrieGraphBTBuilder(TrieGraphBTBuilder &&) = default;
    TrieGraphBTBuilder &operator= (TrieGraphBTBuilder &&) = default;

    TrieGraphData &&build() && {
        auto time_01 = std::chrono::steady_clock::now();

        std::cerr << "BT builder" << std::endl;
        data.letter_loc.init(data.graph);
        // this->data.trie_data.active_trie.insert(Kmer::empty());
        auto &nodes = data.graph.nodes;
        for (NodeLoc i = 0, nsz = nodes.size(); i < nsz; ++i) {
            auto &node = nodes[i];
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

        this->data.trie_data.init(std::move(fast_pairs), this->data.letter_loc);

        auto time_03 = std::chrono::steady_clock::now();
        std::cerr << "HT full: " << std::chrono::duration_cast<std::chrono::milliseconds>(time_03 - time_02).count() << "ms" << std::endl;
        std::cerr << "total: " << std::chrono::duration_cast<std::chrono::milliseconds>(time_03 - time_01).count() << "ms" << std::endl;

        return std::move(data);
    }

    void _back_track(NodePos np) {
        if (this->kmer.is_complete()) {
            auto ll = this->data.letter_loc.compress(np);
            fast_pairs.emplace_back(this->kmer, ll);
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
        this->kmer.push_back(this->data.graph.nodes[np.node].seg[np.pos]);
        if (np.pos + 1 == this->data.graph.nodes[np.node].seg.size()) {
            for (const auto &fwd : this->data.graph.forward_from(np.node)) {
                _back_track(NodePos(fwd.node_id, 0));
            }
            // end-of-graph handling
            if (this->data.graph.edge_start[np.node] == Graph::INV_SIZE &&
                    this->kmer.is_complete()) {
                _back_track(NodePos(this->data.letter_loc.num_locations, 0));
            }
        } else {
            _back_track(NodePos(np.node, np.pos+1));
        }
        this->kmer.pop_back();
    }
};

} /* namespace triegraph */

#endif /* __TRIEGRAPH_BUILDER_BT_H__ */
