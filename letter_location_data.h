#ifndef __LETTER_LOCATION_DATA_H__
#define __LETTER_LOCATION_DATA_H__

template <typename Size_>
struct NodePos {
    using Size = Size_;
    using NodeLoc = Size;
    using LetterLoc = Size;

    NodeLoc node;
    LetterLoc pos;

    NodePos(NodeLoc node = 0, LetterLoc pos = 0) : node(node), pos(pos) {}
    bool operator == (const NodePos &other) const = default;
};

template <typename NodePos_, typename Graph_>
struct LetterLocData {
    using NodePos = NodePos_;
    using Graph = Graph_;
    using NodeLoc = typename NodePos::NodeLoc;
    using LetterLoc = typename NodePos::LetterLoc;

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
        node_start.push_back(num_locations);
    }

    NodeLoc loc2node(LetterLoc loc) {
        return std::lower_bound(node_start.begin(), node_start.end(), loc) - node_start.begin();
    }

    NodePos expand(LetterLoc loc) {
        NodeLoc node = loc2node(loc);
        return NodePos(node, loc - node_start[node]);
    }

    LetterLoc compress(NodePos handle) {
        return handle.node == num_locations ? num_locations :
            node_start[handle.node] + handle.pos;
    }
};

#endif /* __LETTER_LOCATION_DATA_H__ */
