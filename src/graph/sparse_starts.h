#ifndef __SPARSE_STARTS_H__
#define __SPARSE_STARTS_H__

#include "util/util.h"

#include <assert.h>
#include <queue>
#include <unordered_set>
#include <vector>

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

        const SparseStarts *parent;
        NodeLoc n;
        NodeLoc node_id;
        EdgeLoc in_node;

        StartIter() : parent(nullptr), node_id(Graph::INV_SIZE) {}
        StartIter(const SparseStarts &parent, NodeLoc n)
            : parent(&parent),
              n(n),
              node_id(0),
              in_node(n-1 - this->parent->tags[0])
        {
            adjust();
        }

        void adjust() {
            if (in_node >= parent->graph.node(node_id).seg.size())
                ++ node_id;
            while (node_id < parent->graph.num_nodes()) {
                in_node = n-1 - parent->tags[node_id];
                if (in_node < parent->graph.node(node_id).seg.size())
                    break;
                ++ node_id;
            }
            if (node_id >= parent->graph.num_nodes()) {
                node_id = Graph::INV_SIZE;
                in_node = std::numeric_limits<EdgeLoc>::max();
                return;
            }
        }

        reference operator*() const { return NodePos(node_id, in_node); }
        Self& operator++() { in_node += n; adjust(); return *this; }
        Self operator++(int) { Self tmp = *this; ++(*this); return tmp; }
        bool operator== (const Self& other) const { return node_id == other.node_id && in_node == other.in_node; }
        bool operator== (const Sent& other) const { return node_id == Graph::INV_SIZE; }
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

        return StartIter(*this, n);
    }

    void _bfs(NodeLoc n, const std::vector<NodeLoc> &starts) {
        std::queue<NodeLoc> q;
        std::unordered_set<NodeLoc> wait_list;

        for (auto &nl : starts) {
            q.push(nl);
            tags[nl] = n-1; // this could be a very small segment or a loop
                            // point, so starting with a mark is safer and
                            // avoids edge cases
            in_degree[nl] = 0; // 0 means it's visited
        }

        while (q.empty() && wait_list.empty()) {
            NodeLoc crnt;
            if (!q.empty()) {
                crnt = q.front(); q.pop();
            } else {
                assert(!wait_list.empty());
                crnt = wait_list.extract(wait_list.begin()).value();
                tags[crnt] = n-1;
                in_degree[crnt] = 0;
            }

            typename decltype(tags)::value_type next_tag =
                (tags[crnt] + graph.node(crnt).seg.size()) % n;
            for (const auto &fw_node : graph.forward_from(crnt)) {
                if (in_degree[fw_node.node_id] == 0)
                    // already visited, possibly looping back
                    continue;

                tags[fw_node.node_id] = std::max(tags[fw_node.node_id], next_tag);
                if (--in_degree[fw_node.node_id] == 0) {
                    q.push(fw_node.node_id);
                } else {
                    wait_list.insert(fw_node.node_id);
                }
            }
        }
    }
};

} /* namespace triegraph */

#endif /* __SPARSE_STARTS_H__ */
