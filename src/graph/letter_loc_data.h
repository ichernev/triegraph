// SPDX-License-Identifier: MPL-2.0
/*
 * Copyright (c) 2021, Iskren Chernev
 */

#ifndef __LETTER_LOC_DATA_H__
#define __LETTER_LOC_DATA_H__

#include <ostream>

namespace triegraph {

template <typename NodeLoc_, typename NodeLen_ = NodeLoc_>
struct NodePos {
    using NodeLoc = NodeLoc_;
    using NodeLen = NodeLen_;

    NodeLen pos;
    NodeLoc node;

    NodePos(NodeLoc node = 0, NodeLen pos = 0) : pos(pos), node(node) {}
    bool operator == (const NodePos &other) const = default;
    bool operator < (const NodePos &other) const { return node != other.node ? node < other.node : pos < other.pos; }

    friend std::ostream &operator << (std::ostream &os, const NodePos &np) {
        return os << "nodepos:" << np.node << ":" << np.pos;
    }

    NodePos reverse(auto const& graph) const {
        return NodePos(node ^ 1, graph.node(node).seg.size() - 1 - pos);
    }
};

template <typename NodePos_, typename Graph_, typename LetterLoc_, int expand_idx_shift = -1>
struct LetterLocData {
    using NodePos = NodePos_;
    using Graph = Graph_;
    using NodeLoc = NodePos::NodeLoc;
    using NodeLen = NodePos::NodeLen;
    using LetterLoc = LetterLoc_;

    std::vector<LetterLoc> node_start;
    LetterLoc num_locations;

    template <typename, bool>
    struct index_t { };
    template <typename NodeLoc>
    struct index_t<NodeLoc, true> { std::vector<NodeLoc> index; };

    static constexpr bool expand_idx = expand_idx_shift >= 0;
    static constexpr LetterLoc expand_idx_mask = expand_idx ? ((1 << expand_idx_shift) - 1) : 0;
    [[no_unique_address]] index_t<NodeLoc, expand_idx> index;

    LetterLocData() {}
    LetterLocData(const Graph &graph) {
        // + 1 is for the expand_idx temp bound
        node_start.reserve(graph.num_nodes() + 1);
        num_locations = 0;
        for (const auto &node: graph.data.nodes) {
            node_start.push_back(num_locations);
            num_locations += node.seg.length;
        }
        if constexpr (expand_idx) {
            node_start.push_back(num_locations);
            index.index.resize(((num_locations - 1) >> expand_idx_shift) + 1);
            LetterLoc ll = 0;
            for (NodeLoc np = 0; ll < num_locations; ++np)
                for (; ll < node_start[np+1]; ++ll)
                    if ((ll & expand_idx_mask) == 0)
                        index.index[ll >> expand_idx_shift] = np;
            index.index.push_back(node_start.size());
            node_start.pop_back();
        }
    }

    LetterLocData(const LetterLocData &) = delete;
    LetterLocData(LetterLocData &&) = default;
    LetterLocData& operator= (const LetterLocData &) = delete;
    LetterLocData& operator= (LetterLocData &&) = default;

    NodeLoc loc2node(LetterLoc loc) const {
        return std::upper_bound(node_start.begin(), node_start.end(), loc) - node_start.begin() - 1;
    }

    NodePos expand(LetterLoc loc) const {
        if constexpr (expand_idx) {
            NodeLoc lb = index.index[loc >> expand_idx_shift];
            NodeLoc ub = index.index[(loc >> expand_idx_shift) + 1];
            if (ub < node_start.size()) ++ub;
            // std::cerr << loc << " lb " << lb << " ub " << ub << " " << node_start.size() << std::endl;
            NodeLoc node = std::upper_bound(
                    node_start.begin() + lb,
                    node_start.begin() + ub, loc)
                - node_start.begin() - 1;
            return NodePos(node, loc - node_start[node]);
        } else {
            NodeLoc node = loc2node(loc);
            return NodePos(node, loc - node_start[node]);
        }
    }

    LetterLoc compress(NodePos handle) const {
        return node_start[handle.node] + handle.pos;
    }

    struct NPIterSent {};
    struct NPIter {
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = NodePos;
        using reference         = value_type;
        using Self              = NPIter;
        using Sent              = NPIterSent;

        const LetterLocData *parent;
        NodePos np;

        NPIter() : parent(nullptr), np(0, 0) {}
        NPIter(const LetterLocData &parent)
            : parent(&parent),
              np(0, 0) { }

        void adjust() {
            if (np.node + 1 < parent->node_start.size()) [[likely]] {
                if (np.pos == parent->node_start[np.node+1] - parent->node_start[np.node]) {
                    ++ np.node;
                    np.pos = 0;
                }
            } else if (parent->node_start[np.node] + np.pos >= parent->num_locations) {
                np.node = Graph::INV_SIZE;
                np.pos = 0;
            }
        }

        reference operator*() const { return np; }
        Self& operator++() { ++ np.pos; adjust(); return *this; }
        Self operator++(int) { Self tmp = *this; ++(*this); return tmp; }
        bool operator== (const Self& other) const { return np == other.np; }
        bool operator== (const Sent& other) const { return np.node == Graph::INV_SIZE; }
    };

    using const_iterator = NPIter;
    using const_iterator_sent = NPIterSent;

    const_iterator begin() const { return NPIter(*this); }
    const_iterator_sent end() const { return NPIterSent {}; }
};

} /* namespace triegraph */

#endif /* __LETTER_LOC_DATA_H__ */
