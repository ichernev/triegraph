#ifndef __COMPLEXITY_COMPONENT_WALKER_H__
#define __COMPLEXITY_COMPONENT_WALKER_H__

namespace triegraph {

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
              state(this->parent->ccs.empty() ? INCOMING : INTERNAL)

        { }

        reference operator* () const { return NodePos(_node_id(), pos); }
        Self& operator++ () {
            ++ pos;
            if (pos == this->graph->node(_node_id()).seg.size()) {
                ++ n_id;
                if (n_id == (state == INTERNAL ?
                            this->parent->ccs[cc_id].internal.size() :
                            this->parent->incoming.size())) {
                    if (state == INTERNAL) {
                        n_id = 0;
                        pos = 0;
                        ++ cc_id;
                        if (cc_id == this->parent->ccs.size()) {
                            state = INCOMING;
                            n_id = 0;
                            pos = 0;
                            if (!this->parent->incoming.empty())
                                pos = _first_pos();
                        }
                    }
                } else {
                    pos = _first_pos();
                }

            }
            return *this;
        }
        Self operator++(int) { Self tmp = *this; ++(*this); return tmp; }
        bool operator== (const Self& other) const {
            return state == other.state && cc_id == other.cc_id &&
                n_id == other.n_id && pos == other.pos; }
        bool operator== (const Sent& other) const {
            return state == INCOMING && n_id == this->parent->incoming.size(); }

        NodeLoc _node_id() const {
            return state == INTERNAL ?
                this->parent->ccs[cc_id].internal[n_id] :
                this->parent->incoming[n_id];
        }
        NodeLen _first_pos() const {
            return state == INTERNAL ?
                0 :
                graph->node(_node_id()).seg.size() - trie_depth;
        }

        const Parent *parent;
        const Graph *graph;
        u32 trie_depth;
        enum { INTERNAL, INCOMING } state = INTERNAL;
        NodeLoc cc_id = 0;
        NodeLoc n_id = 0;
        NodeLen pos = 0;
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
            if (parent.external.size() == 0) {
                state = INCOMING;
                if (parent.incoming.size() == 0)
                    node_idx = Graph::INV_SIZE;
            }
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
                        node_idx = 0;
                        goto next;
                    }
                } else {
                next:
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

    template <std::ranges::range R>
    struct Builder {
        Builder(const Graph &graph,
                const R &cc_seeds,
                u32 trie_depth)
            : graph(graph),
              cc_seeds(cc_seeds),
              trie_depth(trie_depth)
        {}

        ComplexityComponentWalker build() {
            std::vector<ComplexityComponent> ccs;
            std::vector<bool> in_cc(graph.num_nodes(), false);
            std::vector<bool> in_cci(graph.num_nodes(), false);

            for (NodeLoc cc_seed: cc_seeds) {
                // only "short" nodes can be cc seeds
                if (graph.node(cc_seed).seg.size() >= trie_depth) {
                    std::cerr << "WOOT " << cc_seed << std::endl;
                }
                assert(graph.node(cc_seed).seg.size() < trie_depth);
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
