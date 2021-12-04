// SPDX-License-Identifier: MPL-2.0
/*
 * Copyright (c) 2021, Iskren Chernev
 */

#ifndef __SPARSE_STARTS_H__
#define __SPARSE_STARTS_H__

#include "util/util.h"

#include <assert.h>
#include <queue>
#include <unordered_set>
#include <vector>
#include <algorithm>

namespace triegraph {

template <typename Graph_, typename NodePos_>
struct SparseStarts {
    using Graph = Graph_;
    using NodeLoc = Graph::NodeLoc;
    using NodePos = NodePos_;
    using EdgeLoc = Graph::EdgeLoc;

    const Graph &graph;
    std::vector<u8> tags;
    std::vector<u16> in_degree;

    SparseStarts(const Graph &graph) : graph(graph) {}

    struct StartIterSent {};
    struct StartIter {
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = NodePos;
        using reference         = value_type;
        using Self              = StartIter;
        using Sent              = StartIterSent;

        const SparseStarts *parent; NodeLoc n;
        NodePos np;

        StartIter() : parent(nullptr), np(Graph::INV_SIZE, 0) {}
        StartIter(const SparseStarts &parent, NodeLoc n)
            : parent(&parent),
              n(n),
              np(0, n-1 - this->parent->tags[0])
        {
            adjust();
        }

        void adjust() {
            // std::cerr << "adjusting " << np << std::endl;
            if (np.pos < parent->graph.node(np.node).seg.size())
                return;

            while (++np.node < parent->graph.num_nodes()) {
                np.pos = n-1 - parent->tags[np.node];
                if (np.pos < parent->graph.node(np.node).seg.size())
                    break;
            }
            if (np.node >= parent->graph.num_nodes()) {
                np.node = Graph::INV_SIZE;
                np.pos = 0;
                return;
            }
        }

        reference operator*() const { return np; }
        Self& operator++() { np.pos += n; adjust(); return *this; }
        Self operator++(int) { Self tmp = *this; ++(*this); return tmp; }
        bool operator== (const Self& other) const { return np == other.np; }
        bool operator== (const Sent& other) const { return np.node == Graph::INV_SIZE; }
    };

    using const_iterator = StartIter;
    using const_iterator_sent = StartIterSent;
    using const_iter_view = iter_pair<const_iterator, const_iterator_sent>;

    const_iter_view compute_starts_every(
            NodeLoc n,
            const std::vector<NodeLoc> &starts) {
        assert(n <= std::numeric_limits<typename decltype(tags)::value_type>::max());
        tags.resize(graph.num_nodes(), 0);
        in_degree.resize(graph.num_nodes(), 0);

        for (NodeLoc i = 0, sz = graph.num_nodes(); i < sz; ++i) {
            auto in_deg = std::ranges::distance(graph.backward_from(i));
            assert(in_deg <= std::numeric_limits<typename decltype(in_degree)::value_type>::max());
            in_degree[i] = in_deg;
        }

        _bfs(n, starts);

        // std::ranges::copy(tags, std::ostream_iterator<int>(std::cerr, " ")); std::cerr << std::endl;

        return StartIter(*this, n);
    }

    void _bfs(NodeLoc n, const std::vector<NodeLoc> &starts) {
        // std::cerr << "start " << n << std::endl;
        std::queue<NodeLoc> q;
        std::unordered_set<NodeLoc> wait_list;

        for (auto &nl : starts) {
            q.push(nl);
            tags[nl] = n-1; // this could be a very small segment or a loop
                            // point, so starting with a mark is safer and
                            // avoids edge cases
            in_degree[nl] = 0; // 0 means it's visited
        }

        while (!q.empty() || !wait_list.empty()) {
            NodeLoc crnt;
            if (!q.empty()) {
                crnt = q.front(); q.pop();
                // std::cerr << "popping q " << crnt << std::endl;
            } else {
                assert(!wait_list.empty());
                crnt = wait_list.extract(wait_list.begin()).value();
                tags[crnt] = n-1;
                in_degree[crnt] = 0;
                // std::cerr << "popping wl " << crnt << std::endl;
            }

            typename decltype(tags)::value_type next_tag =
                (tags[crnt] + graph.node(crnt).seg.size()) % n;
            for (const auto &fw : graph.forward_from(crnt)) {
                if (in_degree[fw.node_id] == 0)
                    // already visited, possibly looping back
                    continue;

                tags[fw.node_id] = std::max(tags[fw.node_id], next_tag);
                if (--in_degree[fw.node_id] == 0) {
                    wait_list.erase(fw.node_id);
                    q.push(fw.node_id);
                } else {
                    wait_list.insert(fw.node_id);
                }
            }
        }
    }
};

} /* namespace triegraph */

#endif /* __SPARSE_STARTS_H__ */
