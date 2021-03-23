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
};

} /* namespace triegraph */

#endif /* __LETTER_LOC_DATA_H__ */
