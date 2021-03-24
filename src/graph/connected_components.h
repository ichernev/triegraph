#ifndef __CONNECTED_COMPONENTS_H__
#define __CONNECTED_COMPONENTS_H__

#include <vector>
#include <unordered_set>
#include <queue>

namespace triegraph {

template <typename Graph_>
struct ConnectedComponents {
    using Graph = Graph_;
    using NodeLoc = Graph::NodeLoc;

    const Graph &graph;
    std::vector<NodeLoc> comp_id; // (raw_graph.rgfa_nodes.size(), INV_SIZE);
    NodeLoc num_comp = 0;
    ConnectedComponents(const Graph &graph)
        : graph(graph),
          comp_id(graph.num_nodes(), Graph::INV_SIZE),
          num_comp(0)
    {
        // std::cerr << "==== computing components" << std::endl;
        for (NodeLoc i = 0; i < comp_id.size(); ++i) {
            if (comp_id[i] == Graph::INV_SIZE)
                _bfs_2way(i, num_comp++);
        }
    }

    std::vector<NodeLoc> compute_starting_points() const {
        // std::cerr << "==== computing starts" << std::endl;
        std::vector<NodeLoc> starts;
        std::unordered_set<NodeLoc> done_comps;

        starts.reserve(num_comp);
        // use obvious starting points
        for (NodeLoc i = 0; i < graph.num_nodes(); ++i) {
            if (graph.backward_from(i).empty()) {
                starts.push_back(i);
                done_comps.insert(comp_id[i]);
            }
        }
        // for cyclic components, just add any internal node
        for (NodeLoc i = 0; i < graph.num_nodes(); ++i) {
            if (!done_comps.contains(comp_id[i])) {
                done_comps.insert(comp_id[i]);
                starts.push_back(i);
            }
        }
        return starts;
    }

    void _bfs_2way(NodeLoc start, NodeLoc comp) {
        std::queue<NodeLoc> q;

        q.push(start);
        comp_id[start] = comp;

        while (!q.empty()) {
            auto crnt = q.front(); q.pop();

            for (const auto &to : graph.forward_from(crnt)) {
                if (comp_id[to.node_id] == Graph::INV_SIZE) {
                    comp_id[to.node_id] = comp;
                    q.push(to.node_id);
                }
            }
            for (const auto &from : graph.backward_from(crnt)) {
                if (comp_id[from.node_id] == Graph::INV_SIZE) {
                    comp_id[from.node_id] = comp;
                    q.push(from.node_id);
                }
            }
        }
    }
};

} /* namespace triegraph */

#endif /* __CONNECTED_COMPONENTS_H__ */
