#ifndef __TOP_ORDER_H__
#define __TOP_ORDER_H__

#include <vector>
#include <stack>
#include <algorithm>
#include <assert.h>

namespace triegraph {

template <typename Graph_>
struct TopOrder {
    using Graph = Graph_;
    using NodeLoc = Graph::NodeLoc;
    using EdgeLoc = Graph::EdgeLoc;

    std::vector<NodeLoc> idx;

    TopOrder() {}
    // create via Builder

    TopOrder(const TopOrder &) = delete;
    TopOrder(TopOrder &&) = default;
    TopOrder &operator= (const TopOrder &) = delete;
    TopOrder &operator= (TopOrder &&) = default;

    struct Comparator {
        const TopOrder &top_ord;

        Comparator(const TopOrder &top_ord) : top_ord(top_ord) {}

        bool operator() (const NodeLoc &a, const NodeLoc &b) const {
            return top_ord.idx[a] < top_ord.idx[b];
        }
    };

    Comparator comp() const { return { *this }; }

    std::vector<NodeLoc> get_ordered_nodes() const {
        std::vector<NodeLoc> res(idx.size());
        for (NodeLoc i = 0; i < idx.size(); ++i) {
            res[idx.size() - 1 - idx[i]] = i;
        }
        return res;
    }

    bool is_backedge(typename Graph::EdgeExtra edge) const {
        return idx[edge.from] <= idx[edge.to];
    }

    void sanity_check(const Graph &g, bool dag) const {
        std::vector<NodeLoc> idx_copy = idx;
        std::ranges::sort(idx_copy);
        for (NodeLoc i = 0; i < idx_copy.size(); ++i) {
            assert(idx_copy[i] == i);
        }
        if (dag) {
            for (NodeLoc i = 0; i < g.num_nodes(); ++i) {
                for (const auto &fwd : g.forward_from(i)) {
                    assert(idx[i] > idx[fwd.node_id]);
                }
            }
        }
    }

    struct Builder {
        const Graph &graph;
        std::stack<std::pair<NodeLoc, EdgeLoc>> stk;
        std::vector<bool> in_stk;
        // std::vector<bool> out;

        std::vector<NodeLoc> top_ord;
        NodeLoc top_ord_idx;

        Builder(const Graph &graph)
            : graph(graph),
              in_stk(graph.num_nodes(), false),
              // out(graph.num_nodes(), false),
              top_ord(graph.num_nodes(), Graph::INV_SIZE),
              top_ord_idx(0)
        {}

        TopOrder build() && {
            for (NodeLoc ni = 0; ni < graph.num_nodes(); ++ni)
                if (!in_stk[ni])
                    _dfs(ni);

            return { std::move(top_ord) };
        }

        TopOrder build(std::ranges::input_range auto&& range) && {
            for (const auto &ni : range) {
                assert(!in_stk[ni]);
                _dfs(ni);
            }
            return { std::move(top_ord) };
        }

        void _dfs(NodeLoc start) {
            _push(start);
            // stk.emplace(start,
            //         (*graph.forward_from(start).begin()).edge_id);
            // in_stk[start] = true;

            while (!stk.empty()) {
                auto [ni, ei] = stk.top(); stk.pop();

                if (ei == Graph::INV_SIZE) {
                    top_ord[ni] = top_ord_idx++;
                    // stk.pop();
                } else {
                    auto fwd = graph.forward_edge(ei);
                    stk.emplace(ni, fwd.next_id);

                    if (!in_stk[fwd.to]) {
                        _push(fwd.to);
                        // stk.emplace(fwd.to,
                        //         (*graph.forward_from(fwd.to).begin()).edge_id);
                        // in_stk[fwd.to] = true;
                    }

                    // out[ni] = true;
                }
            }
        }

        void _push(NodeLoc node) {
            auto fwd = graph.forward_from(node);
            EdgeLoc ei = Graph::INV_SIZE;
            if (!fwd.empty()) {
                ei = (*fwd.begin()).edge_id;
            }

            stk.emplace(node, ei);
            in_stk[node] = true;
        }
    };

private:
    // used by Builder
    TopOrder(std::vector<NodeLoc> &&idx) : idx(std::move(idx)) {}

};

} /* namespace triegraph */

#endif /* __TOP_ORDER_H__ */
