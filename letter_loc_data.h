#ifndef __LETTER_LOC_DATA_H__
#define __LETTER_LOC_DATA_H__

#include <ostream>

namespace triegraph {

template <typename NodeLoc_, typename NodeLen_ = NodeLoc_>
struct NodePos {
    using NodeLoc = NodeLoc_;
    using NodeLen = NodeLen_;

    // TODO: Add tests in handle that verify byte order
    NodeLen pos;
    NodeLoc node;

    NodePos(NodeLoc node = 0, NodeLen pos = 0) : pos(pos), node(node) {}
    bool operator == (const NodePos &other) const = default;

    friend std::ostream &operator << (std::ostream &os, const NodePos &np) {
        return os << "nodepos:" << np.node << ":" << np.pos;
    }
};

template <typename NodePos_, typename Graph_, typename LetterLoc_>
struct LetterLocData {
    using NodePos = NodePos_;
    using Graph = Graph_;
    using NodeLoc = NodePos::NodeLoc;
    using NodeLen = NodePos::NodeLen;
    using LetterLoc = LetterLoc_;

    std::vector<LetterLoc> node_start;
    LetterLoc num_locations;

    LetterLocData() {}

    void init(const Graph &graph) {
        node_start.reserve(graph.nodes.size() + 1);
        num_locations = 0;
        for (const auto &node: graph.nodes) {
            node_start.push_back(num_locations);
            num_locations += node.seg.length;
        }
        // node_start.push_back(num_locations);
    }

    NodeLoc loc2node(LetterLoc loc) const {
        return std::upper_bound(node_start.begin(), node_start.end(), loc) - node_start.begin() - 1;
    }

    NodePos expand(LetterLoc loc) const {
        NodeLoc node = loc2node(loc);
        return NodePos(node, loc - node_start[node]);
    }

    LetterLoc compress(NodePos handle) const {
        return handle.node == num_locations ? num_locations :
            node_start[handle.node] + handle.pos;
    }
};

} /* namespace triegraph */

#endif /* __LETTER_LOC_DATA_H__ */
