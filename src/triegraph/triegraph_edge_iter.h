#ifndef __TRIEGRAPH_EDGE_ITER_H__
#define __TRIEGRAPH_EDGE_ITER_H__

#include "util/util.h"

#include <cstring> /* std::memcpy */
#include <utility> /* std::forward */
#include <iterator> /* std::forward_iterator_tag */
#include <ostream>

namespace triegraph {

template <typename Handle_>
struct EditEdge {
    using Handle = Handle_;
    using Letter = Handle::Kmer::Letter;

    Handle handle;
    enum {MATCH, SUB, INS, DEL} edit;
    Letter letter;

    bool operator ==(const EditEdge &) const = default;

    const char *edit_name() const {
        switch (edit) {
            case MATCH: return "MATCH";
            case SUB:   return "SUB";
            case INS:   return "INS";
            case DEL:   return "DEL";
            default:    throw "undefined edit value";
        }
    }

    friend std::ostream &operator<<(std::ostream &os, const EditEdge &ee) {
        return os << ee.handle << " " << ee.edit_name() << " " << ee.letter;
    }
};

template <typename Edge_>
struct EdgeIterImplBase {
    using Edge = Edge_;
    using Handle = Edge::Handle;
    using Letter = Edge::Letter;
    using Self = EdgeIterImplBase;

    u32 state;

    EdgeIterImplBase(u32 state = 0) : state(state) {}

    void inc() {
        ++state;
        after_inc();
    }

    virtual Edge get() const { return Edge{}; }
    virtual u32 end_state() const { return 0; }
    virtual void after_inc() { }
};

// SUB A p1
// SUB G p1
// MATCH C p1
// SUB T p1
//
// INS A p0
// INS C p0
// INS G p0
// INS T p0
//
// DEL EPS p1
template <typename Edge_>
struct EdgeIterImplGraphFwd final : EdgeIterImplBase<Edge_> {
    using Edge = Edge_;
    using Handle = Edge::Handle;
    using Letter = Edge::Letter;

    Letter actual;
    typename Handle::NodePos p0;

    EdgeIterImplGraphFwd(Letter actual, typename Handle::NodePos p0)
        : actual(actual), p0(p0) {}

    virtual u32 end_state() const {
        return 2 * Letter::num_options + 1;
    }

    virtual Edge get() const {
        Letter c { this->state & Letter::mask };
        switch ((this->state >> Letter::bits) & 3) {
            case 0:
                // sub/match
                return Edge {
                    Handle(p0.node, p0.pos + 1),
                    c == actual ? Edge::MATCH : Edge::SUB,
                    c,
                };
            case 1:
                // ins
                return Edge { p0, Edge::INS, c };
            case 2:
                // first one is del, then quit
                if (c.data == 0) {
                    return Edge {
                        Handle(p0.node, p0.pos+1),
                        Edge::DEL,
                        Letter::EPS,
                    };
                } /* else {
                    return end();
                } */
            default:
                throw "WTF bad state";
        }
    }
};


// SUB A p1
// SUB G p1
// MATCH C p1
// SUB T p1
// DEL EPS p1
//
// SUB A p2
// SUB G p2
// MATCH C p2
// SUB T p2
// DEL EPS p2
//
// INS A p0
// INS C p0
// INS G p0
// INS T p0

template <typename Edge_, typename Graph_>
struct EdgeIterImplGraphSplit final : EdgeIterImplBase<Edge_> {
    using Edge = Edge_;
    using Handle = Edge::Handle;
    using Letter = Edge::Letter;

    using Graph = Graph_;

    Letter actual;
    typename Handle::NodePos p0;
    typename Graph::const_iter_view ith;

    EdgeIterImplGraphSplit(Letter actual, typename Handle::NodePos p0,
            typename Graph::const_iter_view ith)
        : actual(actual), p0(p0), ith(ith)
    {
        if (ith.empty()) {
            // skip edit edges generated from it
            this->state = Letter::num_options + 1;
        }
    }

    virtual u32 end_state() const {
        // some states are repeated
        return 2 * Letter::num_options + 1;
    }

    virtual void after_inc() {
        if (this->state == Letter::num_options + 1 && !(++ith).empty()) {
            this->state = 0;
        }
    }

    virtual Edge get() const {
        switch (this->state / (Letter::num_options + 1)) {
            case 0: {
                // std::cerr << "ST " << this->state << " " << (int)c.data << ":" << Letter::num_options << std::endl;
                Letter c { this->state % Letter::num_options };
                auto last = this->state == Letter::num_options;
                return Edge {
                    Handle((*ith).node_id, 0),
                    last ? Edge::DEL : c == actual ? Edge::MATCH : Edge::SUB,
                    last ? Letter {Letter::EPS} : c,
                };
            }
            case 1: {
                Letter c { (this->state - 1) % Letter::num_options };
                if (c.data < Letter::num_options) {
                    return Edge {
                        p0, Edge::INS, c,
                    };
                }
            }
            default:
                throw "WTF bad state";
        }
    }
};

// Kmer00 : kmer02=(kmer00+C), kmer03=(kmer00+G)

// SUB A kmer02
// MATCH C kmer02
// SUB G kmer02
// SUB T kmer02
// DEL EPS kmer02
// --
// SUB A kmer03
// SUB C kmer03
// MATCH G kmer03
// SUB T kmer03
// DEL EPS kmer03
// --
// INS A kmer00
// INS C kmer00
// INS G kmer00
// INS T kmer00

template <typename Edge_, typename OptionsBitset_ = u32>
struct EdgeIterImplTrieInner final : EdgeIterImplBase<Edge_> {
    using Edge = Edge_;
    using Handle = Edge::Handle;
    using Letter = Edge::Letter;
    using Kmer = Handle::Kmer;
    using OptionsBitset = OptionsBitset_;
    static constexpr decltype(EdgeIterImplBase<Edge>::state) num_letters_eps = Letter::num_options + 1;
    // num_letter_eps can be u64, but being same as state helps speed up div
    static_assert(sizeof(num_letters_eps) * BITS_PER_BYTE >= Letter::num_options,
            "not enough space in num_letter_eps");

    Kmer kmer;
    OptionsBitset opts;

    EdgeIterImplTrieInner(Kmer kmer, OptionsBitset opts) : kmer(kmer), opts(opts) {
        after_inc();
    }

    virtual u32 end_state() const {
        // some states are repeated / skipped
        return (num_letters_eps + 1) * Letter::num_options;
    }

    virtual void after_inc() {
        while (this->state < num_letters_eps * Letter::num_options)
            if (auto dr = div(this->state, num_letters_eps);
                    dr.rem == 0 && !(opts & OptionsBitset(1) << dr.quot))
                this->state += num_letters_eps;
            else
                break;
    }

    virtual Edge get() const {
        if (this->state < num_letters_eps * Letter::num_options) {
            auto dr = div(this->state, num_letters_eps);
            Letter k {dr.quot};
            Letter e {dr.rem};
            Kmer nkmer = kmer; nkmer.push(k);
            return Edge {
                nkmer,
                dr.rem == Letter::num_options ? Edge::DEL :
                    k == e ? Edge::MATCH : Edge::SUB,
                e
            };
        } else {
            return Edge {
                kmer,
                Edge::INS,
                Letter { this->state % num_letters_eps }
            };
        }
    }
};

template <typename Edge_, typename TrieGraphData_>
struct EdgeIterImplTrieToGraph final : EdgeIterImplBase<Edge_> {
    using Edge = Edge_;
    using Letter = Edge::Letter;
    using Kmer = Edge::Handle::Kmer;
    using TrieGraphData = TrieGraphData_;
    using Graph = TrieGraphData::Graph;
    using TrieData = TrieGraphData::TrieData;
    using Base = EdgeIterImplBase<Edge>;

    const TrieGraphData &trie_graph;
    typename TrieData::t2g_values_view nps;
    union IterU {
        IterU() : stupid('x') {}
        char stupid;
        EdgeIterImplGraphFwd<Edge> fwd;
        EdgeIterImplGraphSplit<Edge, Graph> split;
    } its;

    EdgeIterImplTrieToGraph(Kmer kmer, const TrieGraphData &tg)
        : trie_graph(tg), nps(trie_graph.trie_data.t2g_values_for(kmer))
    {
        after_inc();
    }

    virtual u32 end_state() const {
        return 1;
    }

    virtual void after_inc() {
        if (this->state == 0) {
            if (!advance_its(true))
                // finish
                ++this->state;
        } else if (this->state == 1) {
            if (advance_its(false))
                // keep it up
                --this->state;
        }
    }

    Base *base() { return reinterpret_cast<Base *>(&its); }
    const Base *base() const { return reinterpret_cast<const Base *>(&its); }

    bool advance_its(bool first_time) {
        if (!first_time) {
            base()->inc();
            if (base()->state != base()->end_state())
                return true;
        }
        if (!nps.empty()) {
            std::cerr << "state 1" << std::endl;
            auto letter_loc = *nps;
            std::cerr << "state 1.1 " << letter_loc << std::endl;
            auto np = trie_graph.letter_loc.expand(letter_loc);
            std::cerr << "np" << np << std::endl;
            const auto &node = trie_graph.graph.nodes[np.node];
            if (np.pos + 1 >= node.seg.size()) {
                std::cerr << "making split " << np.pos + 1 << " " << node.seg.size() << std::endl;
                new(&its.split) EdgeIterImplGraphSplit<Edge, Graph>(
                        np.pos == node.seg.size() ?
                            Letter(Letter::EPS) :
                            node.seg[np.pos],
                        np,
                        trie_graph.graph.forward_from(np.node));
            } else {
                std::cerr << "making fwd" << std::endl;
                new(&its.fwd) EdgeIterImplGraphFwd<Edge>(
                        node.seg[np.pos],
                        np);
            }
            std::cerr << "state 2" << std::endl;
            ++ nps;
            std::cerr << "state 2.1" << std::endl;
            return true;
        }
        return false;
    }

    virtual Edge get() const {
        return base()->get();
    }

};

struct EditEdgeIterSent {};

template <typename Handle_, typename TrieGraphData_>
struct EditEdgeIter {
    using Handle = Handle_;
    using TrieGraphData = TrieGraphData_;
    using Edge = EditEdge<Handle>;
    using Graph = TrieGraphData::Graph;
    using NodePos = typename Handle::NodePos;

    using Self = EditEdgeIter;
    using Sent = EditEdgeIterSent;

    union {
        EdgeIterImplBase<Edge> base;
        EdgeIterImplGraphFwd<Edge> graph_fwd;
        EdgeIterImplGraphSplit<Edge, Graph> graph_split;
        EdgeIterImplTrieInner<Edge> trie_inner;
        EdgeIterImplTrieToGraph<Edge, TrieGraphData> trie_to_graph;
    };

    EditEdgeIter() : base() {}
    EditEdgeIter(const Self &other) { copyFrom(other); }
    EditEdgeIter(Self &&other) { copyFrom(other); }
    Self &operator= (const Self &other) { copyFrom(other); return *this; }
    Self &operator= (Self &&other) { copyFrom(other); return *this; }

    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wclass-memaccess"
    void copyFrom(const Self &other) { std::memcpy(this, &other, sizeof(Self)); }
    #pragma GCC diagnostic pop

    // proxy interface

    using IBase = EdgeIterImplBase<Edge>;
    const IBase *basep() const { return reinterpret_cast<const IBase*>(this); }
    IBase *basep() { return reinterpret_cast<IBase*>(this); }

    Edge get() const { return basep()->get(); }
    void inc() { basep()->inc(); }
    u32 state() const { return basep()->state; }
    u32 end_state() const { return basep()->end_state(); }

    // Iterator interface

    using iterator_category = std::forward_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = Edge;
    using reference_type = value_type;

    reference_type operator* () const { return get(); }
    Self &operator++ () { inc(); return *this; }
    Self operator++ (int) { Self tmp = *this; ++(*this); return tmp; }
    bool operator== (const Self &other) const { return state() == other.state(); }
    bool operator== (const Sent &other) const { return state() == end_state(); }

    // helper factory methods
    template <typename... Args>
    static Self make_graph_fwd(Args&&... args) {
        return Self().set<&Self::graph_fwd>(std::forward<Args>(args)...);
    }
    template <typename... Args>
    static Self make_graph_split(Args&&... args) {
        return Self().set<&Self::graph_split>(std::forward<Args>(args)...);
    }
    template <typename... Args>
    static Self make_trie_inner(Args&&... args) {
        return Self().set<&Self::trie_inner>(std::forward<Args>(args)...);
    }
    template <typename... Args>
    static Self make_trie_to_graph(Args&&... args) {
        return Self().set<&Self::trie_to_graph>(std::forward<Args>(args)...);
    }
    template <auto Self::*field, typename... Args>
    Self& set(Args&&... args) {
        new(&(this->*field)) std::decay<decltype(this->*field)>::type(std::forward<Args>(args)...);
        return *this;
    }

    static Self make(Handle h, const TrieGraphData &tgd) {
        using Kmer = Handle::Kmer;
        using Letter = Kmer::Letter;
        if (h.is_trie()) {
            if (h.depth_in_trie() + 1 < Kmer::K) {
                Kmer nkmer = h.kmer;
                u32 bitset = 0;
                static_assert(Letter::num_options <= sizeof(bitset) * BITS_PER_BYTE);
                for (typename Letter::Holder l = 0; l < Letter::num_options; ++l) {
                    nkmer.push_back(l);
                    if (tgd.trie_data.trie_inner_contains(nkmer)) {
                        bitset |= 1 << l;
                    }
                    nkmer.pop();
                }
                return make_trie_inner(h.kmer, bitset);
            } else if (h.depth_in_trie() + 1 == Kmer::K) {
                Kmer nkmer = h.kmer;
                u32 bitset = 0;
                for (typename Letter::Holder l = 0; l < Letter::num_options; ++l) {
                    nkmer.push_back(l);
                    if (tgd.trie_data.t2g_contains(nkmer)) {
                        bitset |= 1 << l;
                    }
                    nkmer.pop();
                }
                return make_trie_inner(h.kmer, bitset);
            } else {
                return make_trie_to_graph(h.kmer, tgd);
            }
        } else {
            auto &node = tgd.graph.nodes[h.node()];
            if (h.pos() + 1 < node.seg.size()) {
                return make_graph_fwd(node.seg[h.pos()], h.nodepos);
            } else {
                return make_graph_split(
                        h.pos() == node.seg.size() ?
                            Letter(Letter::EPS) :
                            node.seg[h.pos()],
                        h.nodepos,
                        tgd.graph.forward_from(h.node()));
            }
        }
    }
};

} /* namespace triegraph */

#endif /* __EDGE_H__ */
