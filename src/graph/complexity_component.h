// SPDX-License-Identifier: MPL-2.0
/*
 * Copyright (c) 2021, Iskren Chernev
 */

#ifndef __COMPLEXITY_COMPONENT_H__
#define __COMPLEXITY_COMPONENT_H__

#include "util/util.h"

#include <vector>
#include <unordered_set>

namespace triegraph {

template <typename Graph_, typename NodePos_>
struct ComplexityComponent {
    using Graph = Graph_;
    using NodeLoc = Graph::NodeLoc;
    using NodePos = NodePos_;
    using NodeLen = NodePos::NodeLen;

    struct NodeIterSent {};
    struct NodeIter {
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = NodePos;
        using reference         = value_type;
        using Self              = NodeIter;
        using Sent              = NodeIterSent;

        NodeIter() :
            graph(nullptr),
            cc(nullptr),
            trie_depth(0),
            state(INTERNAL),
            node_idx(Graph::INV_SIZE) {}
        NodeIter(const Graph &graph, const ComplexityComponent &cc, u32 trie_depth)
            : graph(&graph), cc(&cc), trie_depth(trie_depth)
        {
            if (this->cc->incoming.size() == 0)
                state = INTERNAL;
            else
                rel_pos = this->graph->node(_node_id()).seg.size() - trie_depth;
        }

        reference operator* () const {
            return NodePos(_node_id(), rel_pos);
        }
        Self &operator++ () {
            ++rel_pos;
            if (graph->node(_node_id()).seg.size() == rel_pos) {
                ++node_idx;
                rel_pos = 0;
                if (state == INCOMING) {
                    if (node_idx == this->cc->incoming.size()) {
                        state = INTERNAL;
                        node_idx = 0;
                    } else {
                        rel_pos = graph->node(_node_id()).seg.size() - trie_depth;
                    }
                } else {
                    if (node_idx == this->cc->internal.size())
                        node_idx = Graph::INV_SIZE;
                }
            }
            return *this;
        }

        Self operator++ (int) { Self tmp = *this; ++(*this); return tmp; }
        bool operator== (const Self &other) const { return state == other.state && node_idx == other.node_idx && rel_pos == other.rel_pos; }
        bool operator== (const Sent &other) const { return node_idx == Graph::INV_SIZE; }

        NodeLoc _node_id() const {
            return state == INCOMING ? cc->incoming[node_idx] : cc->internal[node_idx];
        }

        const Graph *graph;
        const ComplexityComponent *cc;
        u32 trie_depth;
        enum { INCOMING, INTERNAL } state = INCOMING;
        NodeLoc node_idx = 0;
        NodeLen rel_pos = 0;
    };

    using const_nodepos_view = iter_pair<NodeIter, NodeIterSent>;
    const_nodepos_view starts_inside(const Graph &graph, u32 trie_depth) const {
        return { NodeIter(graph, *this, trie_depth) };
    }

    struct Builder {
        Builder(const Graph &graph,
                NodeLoc start,
                u32 trie_depth)
            : graph(graph),
              start(start),
              trie_depth(trie_depth)
        {}

        ComplexityComponent build()  {
            std::vector<NodeLoc> incoming;
            std::vector<NodeLoc> outgoing;
            std::vector<NodeLoc> q;
            std::unordered_set<NodeLoc> in_q;

            q.emplace_back(start);
            in_q.emplace(start);
            NodeLoc qp;
            for (qp = 0; qp < q.size(); ++qp) {
                NodeLoc crnt = q[qp];
                for (const auto &fwd : graph.forward_from(crnt)) {
                    if (_is_short(fwd.node_id)) {
                        if (!in_q.contains(fwd.node_id)) {
                            q.emplace_back(fwd.node_id);
                            in_q.emplace(fwd.node_id);
                        }
                    } else {
                        outgoing.emplace_back(fwd.node_id);
                    }
                }
                for (const auto &bwd : graph.backward_from(crnt)) {
                    if (_is_short(bwd.node_id)) {
                        if (!in_q.contains(bwd.node_id)) {
                            q.emplace_back(bwd.node_id);
                            in_q.emplace(bwd.node_id);
                        }
                    } else {
                        incoming.emplace_back(bwd.node_id);
                    }
                }
            }

            // remove duplicates in incoming, outgoing
            std::ranges::sort(incoming);
            auto ur_inc = std::ranges::unique(incoming);
            incoming.resize(ur_inc.begin() - incoming.begin());
            std::ranges::sort(outgoing);
            auto ur_out = std::ranges::unique(outgoing);
            outgoing.resize(ur_out.begin() - outgoing.begin());

            return { std::move(incoming), std::move(outgoing), std::move(q) };
        }

        bool _is_short(NodeLoc node) const {
            return graph.node(node).seg.size() < trie_depth;
        }

        const Graph &graph;
        NodeLoc start;
        u32 trie_depth;
    };

// private:
    // ComplexityComponent(std::vector<NodeLoc> &&incoming, std::vector<NodeLoc> &&outgoing, std::vector<NodeLoc> &&internal)

    std::vector<NodeLoc> incoming;
    std::vector<NodeLoc> outgoing;
    std::vector<NodeLoc> internal;
};

} /* namespace triegraph */

#endif /* __COMPLEXITY_COMPONENT_H__ */
