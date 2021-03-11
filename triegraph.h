#ifndef __TRIEGRAPH_H__
#define __TRIEGRAPH_H__

#include "edge.h"

#include <utility>
#include <tuple>

namespace triegraph {

template<typename TrieGraphData_>
struct TrieGraph {
    using TrieGraphData = TrieGraphData_;
    using Graph = TrieGraphData::Graph;
    using LetterLocData = TrieGraphData::LetterLocData;
    using TrieData = TrieGraphData::TrieData;
    using NodeLoc = Graph::NodeLoc;
    using LetterLoc = LetterLocData::LetterLoc;
    using Kmer = TrieData::Kmer;
    using Letter = Kmer::Letter;
    using NodePos = LetterLocData::NodePos;
    using TrieSize = TrieData::Kmer::Holder;
    using TrieDepth = TrieData::Kmer::klen_type;
    using Handle = triegraph::Handle<Kmer, NodePos>;
    using EdgeIter = EditEdgeIter<Handle, TrieGraphData>;
    using EdgeIterHelper = EditEdgeIterHelper<EdgeIter>;
    using Self = TrieGraph;

    TrieGraphData data;

    std::tuple<NodeLoc, NodeLoc, LetterLoc> graph_size() const {
        return std::make_tuple(
                data.graph.nodes.size(),
                data.graph.edges.size() / 2,
                data.letter_loc.num_letters);
    }

    std::pair<TrieSize, TrieSize> trie_size() const {
        return std::make_pair(
                // NOTE: This is wrong
                data.trie_data.trie2graph.size(),
                data.trie_data.active_trie.size());
    }

    TrieDepth trie_depth() const {
        return Kmer::K;
    }

    EdgeIterHelper next_edit_edges(Handle h) const {
        if (h.is_trie()) {
            if (h.depth_in_trie() + 1 < Kmer::K) {
                Kmer nkmer = h.kmer;
                u32 bitset = 0;
                for (typename Letter::Holder l = 0; l < Letter::num_options; ++l) {
                    nkmer.push_back(l);
                    if (data.trie_data.active_trie.contains(nkmer)) {
                        bitset |= 1 << l;
                    }
                    nkmer.pop();
                }
                return EdgeIterHelper::make_trie_inner(h.kmer, bitset);
            } else if (h.depth_in_trie() + 1 == Kmer::K) {
                Kmer nkmer = h.kmer;
                u32 bitset = 0;
                for (typename Letter::Holder l = 0; l < Letter::num_options; ++l) {
                    nkmer.push_back(l);
                    if (data.trie_data.trie2graph.contains(nkmer)) {
                        bitset |= 1 << l;
                    }
                    nkmer.pop();
                }
                return EdgeIterHelper::make_trie_inner(h.kmer, bitset);
            } else {
                return EdgeIterHelper::make_trie_to_graph(h.kmer, data);
            }
        } else {
            auto &node = data.graph.nodes[h.node()];
            if (h.pos() + 1 < node.seg.size()) {
                return EdgeIterHelper::make_graph_fwd(node.seg[h.pos()], h.nodepos);
            } else {
                return EdgeIterHelper::make_graph_split(
                        h.pos() == node.seg.size() ?
                            Letter(Letter::EPS) :
                            node.seg[h.pos()],
                        h.nodepos,
                        data.graph.forward_from(h.node()).begin());
            }
        }
    }
};

} /* namespace triegraph */

#endif /* __TRIEGRAPH_H__ */
