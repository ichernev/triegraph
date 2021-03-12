#ifndef __TRIEGRAPH_H__
#define __TRIEGRAPH_H__

#include "edge.h"
#include "triegraph_handle_iter.h"

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
    using Str = Graph::Str;
    using LetterLoc = LetterLocData::LetterLoc;
    using Kmer = TrieData::Kmer;
    using Letter = Kmer::Letter;
    using NodePos = LetterLocData::NodePos;
    using TrieSize = TrieData::Kmer::Holder;
    using TrieDepth = TrieData::Kmer::klen_type;
    using Handle = triegraph::Handle<Kmer, NodePos>;
    using NodeLen = Handle::NodeLen;
    using EdgeIter = EditEdgeIter<Handle, TrieGraphData>;
    using EdgeIterHelper = EditEdgeIterHelper<EdgeIter>;
    using PrevHandleIter = triegraph::PrevHandleIter<Handle, TrieGraphData>;
    using PrevHandleIterHelper = triegraph::PrevHandleIterHelper<Handle, TrieGraphData>;
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

    PrevHandleIterHelper prev_graph_handles(Handle h) const { return prev_graph_handles_it(h); }
    PrevHandleIterHelper prev_trie_handles(Handle h) const { return prev_trie_handles_it(h); }
    PrevHandleIter prev_graph_handles_it(Handle h) const {
        return PrevHandleIter::make_graph(data.graph, h);
    }

    PrevHandleIter prev_trie_handles_it(Handle h) const {
        return PrevHandleIter::make_trie(data, h);
    }

    Handle up_trie_handle(Handle h) const {
        if (!h.is_trie())
            return Handle::invalid();

        h.kmer.pop();
        return h;
    }

    NodeLen next_match_many(Handle h, const Str::View &sv) {
        if (!h.is_valid() || !h.is_graph()) {
            return 0;
        }
        auto nview = data.graph.nodes[h.node()].seg.get_view(h.pos());
        return nview.fast_match(sv);
    }

    Handle exact_short_match(const Str::View &sv) {
        if (sv.size() > trie_depth()) {
            return Handle::invalid();
        }
        auto kmer = Kmer::from_sv(sv);
        auto present = false;
        if (kmer.size() == trie_depth()) {
            present = data.trie_data.trie2graph.contains(kmer);
        } else {
            present = data.trie_data.active_trie.contains(kmer);
        }
        return present ? kmer : Handle::invalid();
    }
};

} /* namespace triegraph */

#endif /* __TRIEGRAPH_H__ */
