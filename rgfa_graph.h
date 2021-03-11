#ifndef __RGFA_GRAPH_H__
#define __RGFA_GRAPH_H__

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <iterator>
#include <unordered_map>
#include <vector>

namespace triegraph {

template<typename Str_>
struct RgfaNode {
    using Str = Str_;

    // RgfaNode(const Str &seg, const std::string &seg_id) : seg(seg), seg_id(seg_id) { }
    RgfaNode(Str &&seg, const std::string &seg_id) : seg(std::move(seg)), seg_id(seg_id) { }
    RgfaNode(Str &&seg, std::string &&seg_id) : seg(std::move(seg)), seg_id(std::move(seg_id)) { }

    RgfaNode(RgfaNode &&) = default;
    RgfaNode& operator=(RgfaNode &&) = default;

    RgfaNode(const RgfaNode &) = delete;
    RgfaNode& operator=(const RgfaNode &) = delete;

    Str seg;
    std::string seg_id;
};

template<typename NodeLoc_, typename EdgeLoc_ = NodeLoc_>
struct RgfaEdge {
    using NodeLoc = NodeLoc_;
    using EdgeLoc = EdgeLoc_;

    RgfaEdge() {}
    RgfaEdge(NodeLoc to, EdgeLoc next) : to(to), next(next) {}

    NodeLoc to;
    EdgeLoc next;
};

template <typename Str_, typename NodeLoc_, typename EdgeLoc_ = NodeLoc_>
struct RgfaGraph {
    using Str = Str_;
    using NodeLoc = NodeLoc_;
    using EdgeLoc = EdgeLoc_;
    using Node = RgfaNode<Str>;
    using Edge = RgfaEdge<NodeLoc, EdgeLoc>;
    static constexpr EdgeLoc INV_SIZE = -1;

    std::vector<Node> nodes; // N
    std::vector<Edge> edges; // M * 2
    std::vector<EdgeLoc> edge_start;
    std::vector<EdgeLoc> redge_start;

    struct IterNode {
        const Str &seg;
        const std::string &seg_id;
        NodeLoc node_id;
        EdgeLoc edge_id;
    };

    struct ConstNodeIter {
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = Node;
        // using pointer           = value_type*;  // or also value_type*
        using reference         = IterNode;
        using Self              = ConstNodeIter;

        ConstNodeIter() : graph(nullptr), edge_id(INV_SIZE) {}
        ConstNodeIter(const RgfaGraph &g, EdgeLoc edge_id) : graph(&g), edge_id(edge_id) {}

        reference operator*() const {
            // TODO: This is a hack, support sentinel
            auto node_id = graph->edges[edge_id == INV_SIZE ? 0 : edge_id].to;
            auto &node = graph->nodes[node_id];
            return IterNode { node.seg, node.seg_id, node_id, edge_id };
        }
        // pointer operator->() const { return &foo; }

        Self& operator++() {
            edge_id = graph->edges[edge_id].next;
            return *this;
        }
        Self operator++(int) { Self tmp = *this; ++(*this); return tmp; }

        friend bool operator== (const Self& a, const Self& b) { return a.edge_id == b.edge_id; }

        const RgfaGraph *graph;
        EdgeLoc edge_id;
    };
    using const_iterator = ConstNodeIter;

    struct NeighbourHelper {
        const_iterator begin() const { return beg; }
        const_iterator end() const { return const_iterator(); }

        NeighbourHelper(const RgfaGraph &g, EdgeLoc edge_id) : beg(g, edge_id) {}
    private:
        const_iterator beg;
    };

    NeighbourHelper forward_from(NodeLoc node_id) const {
        return NeighbourHelper(*this, edge_start[node_id]);
    }

    NeighbourHelper backward_from(NodeLoc node_id) const {
        return NeighbourHelper(*this, redge_start[node_id]);
    }

    void add_reverse_complement() {
        nodes.reserve(nodes.size() * 2);
        edges.reserve(edges.size() * 2);
        edge_start.resize(edge_start.size() * 2, INV_SIZE);
        redge_start.resize(redge_start.size() * 2, INV_SIZE);

        auto split = nodes.size();
        for (NodeLoc i = 0; i < split; ++i) {
            auto &node = nodes[i];
            nodes.emplace_back(node.seg.rev_comp(), "revcomp:" + node.seg_id);
        }
        for (EdgeLoc i = 0, sz = edges.size(); i < sz; i += 2) {
            auto &edge_a = edges[i];
            auto &edge_b = edges[i+1];
            edges.emplace_back(edge_b.to + split, edge_start[edge_a.to + split]);
            edge_start[edge_a.to + split] = edges.size() - 1;
            edges.emplace_back(edge_a.to + split, redge_start[edge_b.to + split]);
            redge_start[edge_b.to + split] = edges.size() - 1;
        }
    }

    struct Builder {
        using Self = Builder;
        std::vector<Node> nodes;
        std::vector<Edge> edges;
        std::vector<NodeLoc> edge_start;
        std::vector<EdgeLoc> redge_start;

        std::unordered_map<std::string, NodeLoc> seg2id;

        bool nodes_done = false;

        Self &add_node(Str &&seg, std::string &&seg_id) {
            if (nodes_done) throw "can not add_node after add_edge";

            nodes.emplace_back(std::move(seg), std::move(seg_id));
            return *this;
        }

        void prep_for_edges() {
            if (nodes_done) return;

            NodeLoc i = 0;
            for (Node &node : nodes) {
                seg2id[node.seg_id] = i++;
            }
            edge_start.resize(nodes.size(), INV_SIZE);
            redge_start.resize(nodes.size(), INV_SIZE);

            nodes_done = true;
        }

        Self &add_edge(const std::string &seg_a, const std::string &seg_b) {
            prep_for_edges();

            NodeLoc a = seg2id[seg_a], b = seg2id[seg_b];
            edges.emplace_back(b, edge_start[a]);
            edge_start[a] = edges.size() - 1;
            edges.emplace_back(a, edge_start[b]);
            redge_start[b] = edges.size() - 1;

            return *this;
        }

        RgfaGraph build() {
            prep_for_edges();
            return RgfaGraph(std::move(*this));
        }
    };

    RgfaGraph(Builder &&b)
        : nodes(std::move(b.nodes)),
          edges(std::move(b.edges)),
          edge_start(std::move(b.edge_start)),
          redge_start(std::move(b.redge_start)) {}

    static RgfaGraph from_file(const std::string &file) {

        std::ifstream io = std::ifstream(file);

        Builder builder;

        std::string line;
        while (std::getline(io, line)) {
            char head;
            std::istringstream iss(line);
            iss >> head;

            switch (head) {
                case 'S': {
                    std::string seg_id;
                    Str seg;
                    iss >> seg_id >> seg;

                    builder.add_node(std::move(seg), std::move(seg_id));
                    break;
                }
                case 'L': {
                    std::string seg_a, seg_b, cigar;
                    char dir_a, dir_b;
                    iss >> seg_a >> dir_a >> seg_b >> dir_b >> cigar;
                    if (dir_a != '+' || dir_b != '+' || cigar != "0M") {
                        throw "does not support fancy links";
                    }
                    builder.add_edge(seg_a, seg_b);

                    break;
                }
            }
        }

        return RgfaGraph(std::move(builder));
    }

    friend std::ostream &operator<< (std::ostream &os, const RgfaGraph &graph) {
        for (NodeLoc i = 0; i < graph.nodes.size(); ++i) {
            os << graph.nodes[i].seg_id << "," << i << " " << graph.nodes[i].seg << " ->";
            for (const auto &to_node : graph.forward_from(i)) {
                os << " " << to_node.seg_id << "," << to_node.node_id;
            }
            os << std::endl;
            // os << graph.nodes[i].seg_id << "," << i << " " << graph.nodes[i].seg " <-";
            // for (const auto &to_node : graph.backward_from(i)) {
            //     os << " " << to_node.seg_id << "," << to_node.node_id;
            // }
            // os << std::endl;
        }
        return os;
    }
};

} /* namespace triegraph */

#endif /* __RGFA_GRAPH_H__ */
