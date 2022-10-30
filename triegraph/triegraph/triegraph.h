// SPDX-License-Identifier: MPL-2.0
/*
 * Copyright (c) 2021, Iskren Chernev
 */

#ifndef __TRIEGRAPH_H__
#define __TRIEGRAPH_H__

#include "triegraph/triegraph/handle.h"
#include "triegraph/triegraph/triegraph_edge_iter.h"
#include "triegraph/triegraph/triegraph_handle_iter.h"

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
    using EditEdgeIter = triegraph::EditEdgeIter<Handle, TrieGraphData>;
    using PrevHandleIter = triegraph::PrevHandleIter<Handle, TrieGraphData>;
    using PrevHandleIterHelper = triegraph::PrevHandleIterHelper<Handle, TrieGraphData>;
    using Self = TrieGraph;

    TrieGraphData data;

    TrieGraph(TrieGraphData &&data) : data(std::move(data)) {}

    std::tuple<NodeLoc, NodeLoc, LetterLoc> graph_size() const {
        return std::make_tuple(
                data.graph.num_nodes(),
                data.graph.num_edges() / 2,
                data.letter_loc.num_locations);
    }

    std::tuple<TrieSize, u32, TrieSize> trie_size() const {
        return std::make_tuple(0, 0, 0);
                // data.trie_data.trie2graph.size(),
                // data.trie_data.trie2graph.key_size(),
                // data.trie_data.active_trie.size());
    }

    TrieDepth trie_depth() const {
        return Kmer::K;
    }

    using edit_edge_iterator = EditEdgeIter;
    using edit_edge_iter_view = iter_pair<
        edit_edge_iterator,
        typename edit_edge_iterator::Sent>;

    Handle root_handle() const {
        return Handle(Kmer::empty());
    }

    edit_edge_iter_view next_edit_edges(Handle h) const { return EditEdgeIter::make(h, data); }
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

        h.kmer().pop();
        return h;
    }

    Handle reverse(Handle h) const {
        return h.nodepos().reverse(data.graph);
    }

    NodeLen next_match_many(Handle h, const Str::View &sv) {
        if (!h.is_valid() || !h.is_graph()) {
            return 0;
        }
        auto nview = data.graph.node(h.node()).seg.get_view(h.pos());
        return nview.fast_match(sv);
    }

    Handle exact_short_match(const Str::View &sv) {
        if (sv.size() > trie_depth()) {
            return Handle::invalid();
        }
        auto kmer = Kmer::from_sv(sv);
        return data.trie_data.trie_contains(kmer) ? kmer : Handle::invalid();
    }
};

} /* namespace triegraph */

#endif /* __TRIEGRAPH_H__ */
