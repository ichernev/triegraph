// SPDX-License-Identifier: MPL-2.0
/*
 * Copyright (c) 2021, Iskren Chernev
 */

#ifndef __RGFA_GRAPH_H__
#define __RGFA_GRAPH_H__

#include "util/util.h"
#include "util/logger.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <iterator>
#include <unordered_map>
#include <vector>
#include <optional>

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

    struct GraphData {
        std::vector<Node> nodes; // N
        std::vector<Edge> edges; // M * 2
        std::vector<EdgeLoc> edge_start;  // N
        std::vector<EdgeLoc> redge_start; // N
    } data;

    struct Settings {
        bool add_reverse_complement = true;
        bool add_extends = true;
    } settings;

    RgfaGraph(GraphData &&d, Settings s)
        : data(std::move(d)),
          settings(s) {
    }

    NodeLoc num_nodes() const { return data.nodes.size(); }
    EdgeLoc num_edges() const { return data.edges.size(); }
    const Node &node(NodeLoc id) const { return data.nodes[id]; }

    struct IterNode {
        const Str &seg;
        const std::string &seg_id;
        NodeLoc node_id;
        EdgeLoc edge_id;
    };

    struct ConstNodeIterSent {};
    struct ConstNodeIter {
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = IterNode;
        using reference         = value_type;
        using Self              = ConstNodeIter;
        using Sent              = ConstNodeIterSent;

        ConstNodeIter() : graph(nullptr), edge_id(INV_SIZE) {}
        ConstNodeIter(const GraphData &g, EdgeLoc edge_id) : graph(&g), edge_id(edge_id) {}

        reference operator*() const {
            auto node_id = graph->edges[edge_id].to;
            auto &node = graph->nodes[node_id];
            return IterNode { node.seg, node.seg_id, node_id, edge_id };
        }

        Self& operator++() { edge_id = graph->edges[edge_id].next; return *this; }
        Self operator++(int) { Self tmp = *this; ++(*this); return tmp; }

        bool operator== (const Self& other) const { return edge_id == other.edge_id; }
        bool operator== (const Sent& other) const { return edge_id == INV_SIZE; }

        const GraphData *graph;
        EdgeLoc edge_id;
    };
    using const_iterator = ConstNodeIter;
    using const_iterator_sent = ConstNodeIterSent;
    using const_iter_view = iter_pair<const_iterator, const_iterator_sent>;
    static_assert(sizeof(const_iterator) == sizeof(const_iter_view));

    const_iter_view forward_from(NodeLoc node_id) const {
        return const_iterator {this->data, data.edge_start[node_id]};
    }

    std::optional<NodeLoc> forward_one(NodeLoc node_id) const {
        auto edge_id = data.edge_start[node_id];
        if (edge_id != INV_SIZE && data.edges[edge_id].next == INV_SIZE) {
            return { data.edges[edge_id].to };
        }
        return {};
    }

    const_iter_view backward_from(NodeLoc node_id) const {
        return const_iterator {this->data, data.redge_start[node_id]};
    }

    struct EdgeExtra {
        NodeLoc from;
        NodeLoc to;
        EdgeLoc edge_id;
        EdgeLoc next_id;
    };

    bool is_reverse_edge(EdgeLoc edge_id) const { return edge_id & 1; }
    EdgeExtra forward_edge(EdgeLoc edge_id) const {
        return {
            .from = data.edges[edge_id ^ 1].to,
            .to = data.edges[edge_id].to,
            .edge_id = edge_id,
            .next_id = data.edges[edge_id].next,
        };
    }
    EdgeExtra reverse_edge(EdgeLoc edge_id) const {
        return forward_edge(edge_id ^ 1);
    }

    struct ConstEdgeIterSent {};
    struct ConstEdgeIter {
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = EdgeExtra;
        using reference         = value_type;
        using Self              = ConstEdgeIter;
        using Sent              = ConstEdgeIterSent;

        ConstEdgeIter() : graph(nullptr), edge_id(INV_SIZE) {}
        ConstEdgeIter(const RgfaGraph &g, EdgeLoc edge_id) : graph(&g), edge_id(edge_id) {}

        reference operator*() const {
            return graph->forward_edge(edge_id);
        }

        Self& operator++() { edge_id += 2; return *this; }
        Self operator++(int) { Self tmp = *this; ++(*this); return tmp; }

        bool operator== (const Self& other) const { return edge_id == other.edge_id; }
        bool operator== (const Sent& other) const { return edge_id >= graph->data.edges.size(); }

        const RgfaGraph *graph;
        EdgeLoc edge_id;
    };

    using const_edge_iterator = ConstEdgeIter;
    using const_edge_iterator_sent = ConstEdgeIterSent;
    using const_edge_iter_view = iter_pair<const_edge_iterator, const_edge_iterator_sent>;
    const_edge_iter_view forward_edges() const { return const_edge_iterator { *this, 0 }; }
    const_edge_iter_view reverse_edges() const { return const_edge_iterator { *this, 1 }; }

    struct Builder {
        using Self = Builder;

        GraphData data;
        Settings settings;

        // std::vector<Node> nodes;
        // std::vector<Edge> edges;
        // std::vector<EdgeLoc> edge_start;
        // std::vector<EdgeLoc> redge_start;

        std::unordered_map<std::string, NodeLoc> seg2id;

        enum { NODES, EDGES, YOLO } state = NODES;
        enum dir { FWD = 0, REVCOMP = 1 };

        dir dir2enum(char d) const {
            return d == '+' ? FWD : REVCOMP;
        }
        dir rev(dir d) const {
            return d == FWD ? REVCOMP : FWD;
        }
        std::string segid(const std::string &seg_id, dir d) const {
            return d == FWD ? seg_id : "revcomp:" + seg_id;
        }

        // regular builder, start from scratch
        Builder(Settings settings = {}) : settings(settings), state(NODES) {}

        Self &add_node(Str &&seg, std::string &&seg_id, bool adjust_edges = false) {
            if (state == EDGES) throw "can not add_node after add_edge";

            data.nodes.emplace_back(std::move(seg), std::move(seg_id));
            if (adjust_edges) {
                data.edge_start.emplace_back(INV_SIZE);
                data.redge_start.emplace_back(INV_SIZE);
            }
            if (settings.add_reverse_complement) {
                const auto &node = data.nodes.back();
                auto seg_id = segid(node.seg_id, REVCOMP);
                // if (seg2id.contains(seg_id)) throw "original nodes contain revcomp: prefix";
                data.nodes.emplace_back(node.seg.rev_comp(), std::move(seg_id));
                if (adjust_edges) {
                    data.edge_start.emplace_back(INV_SIZE);
                    data.redge_start.emplace_back(INV_SIZE);
                }
            }
            return *this;
        }

        void prep_for_edges() {
            if (state != NODES) return;

            NodeLoc i = 0;
            for (Node &node : data.nodes) {
                // TODO: Add check for existing seg_id
                seg2id[node.seg_id] = i++;
            }
            data.edge_start.resize(data.nodes.size(), INV_SIZE);
            data.redge_start.resize(data.nodes.size(), INV_SIZE);

            state = EDGES;
        }

        Self &add_edge(const std::string &seg_a, const std::string &seg_b) {
            prep_for_edges();

            return add_edge(seg_a, '+', seg_b, '+');
        }

        Self &add_edge(const std::string &seg_a, char dir_a,
                const std::string &seg_b, char dir_b) {
            prep_for_edges();

            return add_edge(seg_a, dir2enum(dir_a), seg_b, dir2enum(dir_b));
        }

        // Self &add_reverse_complement() {
        //     prep_for_edges();
        //     state = YOLO;
        //     applied.add_reverse_complement = true;

        //     data.nodes.reserve(data.nodes.size() * 2);
        //     data.edges.reserve(data.edges.size() * 2);
        //     data.edge_start.resize(data.edge_start.size() * 2, INV_SIZE);
        //     data.redge_start.resize(data.redge_start.size() * 2, INV_SIZE);

        //     auto split = data.nodes.size();
        //     for (NodeLoc i = 0; i < split; ++i) {
        //         auto &node = data.nodes[i];
        //         auto id = "revcomp:" + node.seg_id;
        //         if (seg2id.contains(id)) throw "original nodes contain revcomp: prefix";
        //         add_node(node.seg.rev_comp(), std::move(id));
        //     }
        //     for (EdgeLoc i = 0, sz = data.edges.size(); i < sz; i += 2) {
        //         auto &edge_a = data.edges[i];
        //         auto &edge_b = data.edges[i+1];
        //         add_edge(edge_a.to + split, edge_b.to + split);
        //     }

        //     return *this;
        // }

        RgfaGraph build() {
            prep_for_edges();
            if (settings.add_extends)
                add_extends();

            return RgfaGraph(std::move(data), settings);
        }

        // RgfaGraph build(Settings s) {
        //     // make sure settings and applied settings are not out of sync
        //     if (applied.add_reverse_complement && !s.add_reverse_complement)
        //         throw "can NOT undo add_reverse_complement";
        //     if (!applied.add_reverse_complement && s.add_reverse_complement)
        //         add_reverse_complement();
        //     if (applied.add_extends && !s.add_extends)
        //         throw "can NOT undo add_extends";
        //     if (!applied.add_extends && s.add_extends)
        //         add_extends();

        //     return build();
        // }
    private:
        Self &add_edge(const std::string &seg_a, dir dir_a,
                const std::string &seg_b, dir dir_b) {
            if (settings.add_reverse_complement) {
                add_edge(seg2id[segid(seg_a, dir_a)],
                        seg2id[segid(seg_b, dir_b)]);
                add_edge(seg2id[segid(seg_b, rev(dir_b))],
                        seg2id[segid(seg_a, rev(dir_a))]);
            } else {
                if (dir_a == FWD && dir_b == FWD)
                    add_edge(seg2id[seg_a], seg2id[seg_b]);
                else if (dir_a == REVCOMP && dir_b == REVCOMP)
                    add_edge(seg2id[seg_b], seg2id[seg_a]);
                else
                    throw "mismatch directions and no revcomp in settings";
            }
            return *this;
        }

        Self &add_edge(NodeLoc a, NodeLoc b) {
            data.edges.emplace_back(b, data.edge_start.at(a));
            data.edge_start.at(a) = data.edges.size() - 1;
            data.edges.emplace_back(a, data.redge_start.at(b));
            data.redge_start.at(b) = data.edges.size() - 1;

            return *this;
        }

        Self &add_extends() {
            prep_for_edges();
            state = YOLO;
            settings.add_extends = true;

            for (NodeLoc i = 0, sz = data.nodes.size(); i < sz; ++i) {
                if (data.edge_start[i] == INV_SIZE) {
                    auto id = "extend:" + data.nodes[i].seg_id;
                    if (seg2id.contains(id)) throw "original nodes contain extend: prefix";
                    add_node(Str("a"), std::move(id), true);
                    add_edge(i, data.nodes.size() - 1);
                }
            }

            return *this;
        }
    };

    static RgfaGraph from_file(const std::string &file, Settings settings = {}) {
        auto ts = Logger::get().begin_scoped("reading graph");
        std::ifstream io = std::ifstream(file);

        Builder builder(settings);

        auto lfile = to_lower(file);
        if (lfile.ends_with(".gfa") || lfile.ends_with(".rgfa")) {
            from_gfa(io, builder);
        } else if (lfile.ends_with(".fa") || lfile.ends_with(".fasta")) {
            from_fasta(io, builder);
        }

        auto res = builder.build();
        if (res.num_nodes() == 0) {
            throw "empty-graph";
        }
        return res;
    }

    static void from_gfa(std::istream &io, Builder &builder) {
        std::string line;
        // log.begin("reading nodes");
        bool reading_nodes = true;
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
                    if (reading_nodes) {
                        // log.end(); log.begin("reading edges");
                        reading_nodes = false;
                    }
                    std::string seg_a, seg_b, cigar;
                    char dir_a, dir_b;
                    iss >> seg_a >> dir_a >> seg_b >> dir_b >> cigar;
                    if (cigar != "0M") {
                        throw "does not support non-zero overlap";
                    }
                    // if (!settings.add_reverse_complement && dir_a != dir_b)
                    //     throw "directions differ, but no reverse complement in settings";

                    builder.add_edge(seg_a, dir_a, seg_b, dir_b);

                    break;
                }
            }
        }

        // log.end(); log.begin("building");
        // auto res = builder.build();
        // return res;
    }

    static void from_fasta(std::istream &io, Builder &builder) {
        std::string line;
        std::string seg_id;
        std::string plain_seg;
        auto finish_segment = [&builder, &seg_id, &plain_seg]() mutable {
            if (!seg_id.empty() && !plain_seg.empty()) {
                try {
                    Str seg;
                    std::istringstream(plain_seg) >> seg;
                    builder.add_node(std::move(seg), std::move(seg_id));
                } catch (const char *err) {
                    std::cerr << "ERROR: " << err << std::endl;
                    throw;
                }
            }
            plain_seg.clear();
        };

        while (std::getline(io, line)) {
            if (line[0] == '>') {
                finish_segment();
                seg_id = line.substr(1);
            } else if (line[0] == ';') {
                continue;
            } else {
                // used to strip whitespace at the end
                // NOTE: if there are whitespace in the middle it will fuck it
                // up
                std::string piece;
                std::istringstream(line) >> piece;
                plain_seg += piece;
            }
        }
        finish_segment();
    }

    friend std::ostream &operator<< (std::ostream &os, const RgfaGraph &graph) {
        for (NodeLoc i = 0; i < graph.num_nodes(); ++i) {
            os << graph.node(i).seg_id << "," << i << " " << graph.node(i).seg << " ->";
            for (const auto &to_node : graph.forward_from(i)) {
                os << " " << to_node.seg_id << "," << to_node.node_id;
            }
            os << std::endl;
        }
        std::cerr << "REVERSE" << std::endl;
        for (NodeLoc i = 0; i < graph.num_nodes(); ++i) {
            os << graph.node(i).seg_id << "," << i << " " << graph.node(i).seg << " <-";
            for (const auto &to_node : graph.backward_from(i)) {
                os << " " << to_node.seg_id << "," << to_node.node_id;
            }
            os << std::endl;
        }
        return os;
    }
};

} /* namespace triegraph */

#endif /* __RGFA_GRAPH_H__ */
