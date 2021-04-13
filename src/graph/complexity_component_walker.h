#ifndef __COMPLEXITY_COMPONENT_WALKER_H__
#define __COMPLEXITY_COMPONENT_WALKER_H__

namespace triegraph {

// struct JoinerSent {};
// template <typename Rng1, typename GetRng2, typename Rng2>
// struct Joiner {
//     using iterator_category = std::forward_iterator_tag;
//     using difference_type   = std::ptrdiff_t;
//     using value_type        = Rng2::value_type;
//     using reference         = value_type;
//     using Self              = Joiner;
//     using Sent              = JoinerSent;

//     Rng1 rng1;
//     GetRng2 get_rng2;
//     Rng2 rng2;

//     Joiner() {}

//     reference operator* () const { return *rng2; }
//     Self &operator++ () {
//         ++ rng2;
//         while (!rng1.empty() && rng2.empty()) {
//             ++rng1;
//             rng2 = get_rng2(*rng1);
//         }
//     }
//     Self operator++ (int) { Self tmp = *this; ++(*this); return tmp; }
//     bool operator== (const Joiner &other) const { return rng1.begin() == other.begin() && rng2.begin() == other.rng2.begin(); }
//     bool operator== (const JoinerSent &other) const { return rng1.empty(); }
// };

// template <typename Graph, typename ConnectedComponents>
// struct GetStartsInside {
//     const Graph &graph;
//     u32 trie_depth;

//     GetStartsInside(const Graph &graph, u32 trie_depth)
//         : graph(graph),
//           trie_depth(trie_depth)
//     {}

//     typename ConnectedComponents::const_nodeloc_view operator() (
//             const ConnectedComponents &cc) const {
//         return cc.starts_inside(graph, trie_depth);
//     }
// };

// struct

// template <typename ComplexityComponent_, typename ComplexityEstimator_>
template <typename ComplexityComponent_>
struct ComplexityComponentWalker {
    using ComplexityComponent = ComplexityComponent_;
    using Graph = ComplexityComponent::Graph;
    using NodePos = ComplexityComponent::NodePos;
    using NodeLoc = ComplexityComponent::NodeLoc;
    using NodeLen = ComplexityComponent::NodeLen;

    std::vector<ComplexityComponent> ccs;
    std::vector<NodeLoc> external;
    std::vector<NodeLoc> incoming;

    struct InnerIterSent {};
    struct InnerIter {
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = NodePos;
        using reference         = value_type;
        using Self              = InnerIter;
        using Sent              = InnerIterSent;
        using Parent            = ComplexityComponentWalker;

        InnerIter() : parent(nullptr), graph(nullptr) {}
        InnerIter(const Parent &parent, const Graph &graph, u32 trie_depth)
            : parent(&parent),
              graph(&graph),
              trie_depth(trie_depth),
              cc_id(0),
              ip(parent.ccs[cc_id].starts_inside(graph, trie_depth))
        {}

        reference operator* () const { return *ip; }
        Self& operator++ () {
            ++ ip;
            if (ip.empty()) {
                ++ cc_id;
                if (cc_id < this->parent->ccs.size())
                    ip = this->parent->ccs[cc_id].starts_inside(*graph, trie_depth);
            }
            return *this;
        }
        Self operator++(int) { Self tmp = *this; ++(*this); return tmp; }
        bool operator== (const Self& other) const { return cc_id == other.cc_id; }
        bool operator== (const Sent& other) const { return cc_id == this->parent->ccs.size(); }

        const Parent *parent;
        const Graph *graph;
        u32 trie_depth;
        NodeLoc cc_id;
        ComplexityComponent::const_nodepos_view ip;
    };

    iter_pair<InnerIter, InnerIterSent> cc_starts(
            const Graph &graph, u32 trie_depth) const {
        return { InnerIter(*this, graph, trie_depth) };
    }

    struct OuterIterSent {};
    struct OuterIter {
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = NodePos;
        using reference         = value_type;
        using Self              = OuterIter;
        using Sent              = OuterIterSent;
        using Parent            = ComplexityComponentWalker;

        OuterIter() : node_idx(Graph::INV_SIZE) {}
        OuterIter(const Parent &parent, const Graph &graph, u32 trie_depth)
            : parent(&parent),
              graph(&graph),
              trie_depth(trie_depth)
        {
            _adjust();
        }

        reference operator* () const { return NodePos(_node_id(), rel_pos); }
        Self &operator++ () { ++ rel_pos; _adjust(); return *this; }
        Self operator++ (int) { Self tmp = *this; ++(*this); return tmp; }
        // bool operator== (const Self &other) const {
        //     return state == other.state && node_idx == other.node_idx &&
        //         rel_pos == other.rel_pos; }
        bool operator== (const Sent &other) const { return node_idx == Graph::INV_SIZE; }

        NodeLoc _node_id() const {
            return state == EXTERNAL ?
                parent->external[node_idx] :
                parent->incoming[node_idx];
        }

        NodeLen _last_pos() const {
            return state == EXTERNAL ?
                graph->node(_node_id()).seg.size() :
                graph->node(_node_id()).seg.size() - trie_depth;
        }

        void _adjust() {
            while (node_idx != Graph::INV_SIZE && rel_pos >= _last_pos()) {
                ++ node_idx;
                rel_pos = 0;
                if (state == EXTERNAL) {
                    if (node_idx == parent->external.size()) {
                        state = INCOMING;
                        node_idx = -1;
                    }
                } else {
                    if (node_idx == parent->incoming.size()) {
                        node_idx = Graph::INV_SIZE;
                    }
                }
            }
        }

        const Parent *parent;
        const Graph *graph;
        u32 trie_depth;
        enum { EXTERNAL, INCOMING } state = EXTERNAL;
        NodeLoc node_idx = 0;
        NodeLen rel_pos = 0;
    };

    iter_pair<OuterIter, OuterIterSent> non_cc_starts(
            const Graph &graph, u32 trie_depth) const {
        return { OuterIter(*this, graph, trie_depth) };
    }

    template <std::ranges::input_range R>
    struct Builder {
        Builder(const Graph &graph,
                R const& cc_seeds,
                u32 trie_depth)
            : graph(graph),
              cc_seeds(cc_seeds),
              trie_depth(trie_depth)
        {}

        ComplexityComponentWalker build() {
            std::vector<ComplexityComponent> ccs;
            std::vector<bool> in_cc(graph.num_nodes(), false);
            std::vector<bool> in_cci(graph.num_nodes(), false);

            for (const auto &cc_seed: cc_seeds) {
                if (!in_cc[cc_seed]) {
                    ccs.emplace_back(typename ComplexityComponent::Builder(
                                graph, cc_seed, trie_depth).build());
                    for (const auto &inner : ccs.back().internal) {
                        in_cc[inner] = true;
                    }
                    for (const auto &inc : ccs.back().incoming) {
                        in_cci[inc] = true;
                    }
                }
            }

            std::vector<NodeLoc> external;
            std::vector<NodeLoc> incoming;
            for (NodeLoc i = 0; i < graph.num_nodes(); ++i) {
                if (in_cci[i]) { incoming.emplace_back(i); }
                else if (!in_cc[i]) { external.emplace_back(i); }
            }

            return {
                .ccs = std::move(ccs),
                .external = std::move(external),
                .incoming = std::move(incoming)
            };
        }

        const Graph &graph;
        const R &cc_seeds;
        const u32 trie_depth;
    };
};


} /* namespace triegraph */

#endif /* __COMPLEXITY_COMPONENT_WALKER_H__ */
