#ifndef __TOP_ORDER_H__
#define __TOP_ORDER_H__

#include <vector>
#include <stack>

namespace triegraph {

template <typename Graph_>
struct TopOrder {
    using Graph = Graph_;
    using NodeLoc = Graph::NodeLoc;

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
        std::stack<NodeLoc> stk;
        std::vector<bool> in_stk;
        std::vector<bool> out;

        std::vector<NodeLoc> top_ord;
        NodeLoc top_ord_idx;

        Builder(const Graph &graph)
            : graph(graph),
              in_stk(graph.num_nodes(), false),
              out(graph.num_nodes(), false),
              top_ord(graph.num_nodes(), Graph::INV_SIZE),
              top_ord_idx(0)
        {}

        TopOrder build() && {
            for (NodeLoc ni = 0; ni < graph.num_nodes(); ++ni)
                if (!in_stk[ni])
                    _dfs(ni);

            return { std::move(top_ord) };
        }

        void _dfs(NodeLoc start) {
            stk.push(start);
            in_stk[start] = true;

            while (!stk.empty()) {
                NodeLoc ni = stk.top();

                if (out[ni] == true) {
                    top_ord[ni] = top_ord_idx++;
                    stk.pop();
                } else {
                    for (const auto &fwd : graph.forward_from(ni))
                        if (!in_stk[fwd.node_id]) {
                            stk.push(fwd.node_id);
                            in_stk[fwd.node_id] = true;
                        }
                    out[ni] = true;
                }
            }
        }
    };

private:
    // used by Builder
    TopOrder(std::vector<NodeLoc> &&idx) : idx(std::move(idx)) {}

};

} /* namespace triegraph */

#endif /* __TOP_ORDER_H__ */
