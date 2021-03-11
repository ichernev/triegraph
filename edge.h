#ifndef __EDGE_H__
#define __EDGE_H__

#include "util.h"

#include <cstring> /* std::memcpy */
#include <utility> /* std::forward */
#include <iterator> /* std::forward_iterator_tag */
#include <ostream>

template <typename Kmer_, typename NodePos_, typename Size_>
struct Handle {
    using Kmer = Kmer_;
    using NodePos = NodePos_;
    using Size = Size_;
    using NodeLoc = Size;
    using LetterLoc = Size;
    using TrieDepth = typename Kmer::klen_type;

    union {
        Kmer kmer;
        NodePos nodepos;
    };

    Handle() : nodepos(0, 0) {}
    Handle(Kmer k) : kmer(k) {}
    Handle(NodeLoc n, LetterLoc p = 0) : nodepos(n, p) {}
    Handle(NodePos np) : nodepos(np) {}

    bool is_trie() const { return kmer.data & Kmer::ON_MASK; }
    TrieDepth depth() const { return kmer.get_len(); }

    bool is_graph() const { return !is_trie(); }
    NodeLoc node() const { return nodepos.node; }
    LetterLoc pos() const { return nodepos.pos; }

    bool operator== (const Handle &other) const {
        if constexpr (sizeof(kmer) == sizeof(nodepos))
            return kmer == other.kmer;

        if (is_trie()) {
            if (other.is_trie())
                return kmer == other.kmer;
            return false;
        } else {
            if (other.is_graph())
                return nodepos == other.nodepos;
            return false;
        }
    }

    friend std::ostream &operator<< (std::ostream &os, const Handle &h) {
        if (h.is_trie())
            os << "trie:" << h.depth() << ":" << h.kmer;
        else
            os << "graph:" << h.node() << ":" << h.pos();
        return os;
    }
};

template <typename Handle_, typename Letter_>
struct EditEdge {
    using Handle = Handle_;
    using Letter = Letter_;

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
    typename Graph::const_iterator it;

    EdgeIterImplGraphSplit(Letter actual, typename Handle::NodePos p0,
            typename Graph::const_iterator it)
        : actual(actual), p0(p0), it(it)
    {
        // TODO: Fix this hack
        if ((*it).edge_id == Graph::INV_SIZE) {
            // skip edit edges generated from it
            this->state = Letter::num_options + 1;
        }

    }

    virtual u32 end_state() const {
        // some states are repeated
        return 2 * Letter::num_options + 1;
    }

    virtual void after_inc() {
        // TODO: This is a hack (together with ++it code
        if (this->state == Letter::num_options + 1 &&
                (*++it).edge_id != Graph::INV_SIZE) {
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
                    Handle((*it).node_id, 0),
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
    using T2GMap = decltype(TrieData::trie2graph);
    using T2GMapIt = T2GMap::const_iterator;
    using Base = EdgeIterImplBase<Edge>;

    const TrieGraphData &trie_graph;
    T2GMapIt nps;
    union {
        char stupid;
        EdgeIterImplGraphFwd<Edge> fwd;
        EdgeIterImplGraphSplit<Edge, Graph> split;
    } its;

    EdgeIterImplTrieToGraph(Kmer kmer, const TrieGraphData &tg)
        : trie_graph(tg), nps(trie_graph.trie_data.trie2graph.find(kmer)), its{'x'}
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
        // TODO: Do we NEED end-of-graph sentinels in trie2graph?
        // if we hit the end-of-graph sentinel, skip it
        // while (nps != T2GMapIt() &&
        //         nps->second == trie_graph.letter_loc.num_locations) {
        //     ++nps;
        // }
        if (nps != T2GMapIt()) {
            auto letter_loc = nps->second;
            auto np = trie_graph.letter_loc.expand(letter_loc);
            const auto &node = trie_graph.graph.nodes[np.node];
            if (np.pos + 1 >= node.seg.size()) {
                new(&its.split) EdgeIterImplGraphSplit<Edge, Graph>(
                        np.pos == node.seg.size() ?
                            Letter(Letter::EPS) :
                            node.seg[np.pos],
                        np,
                        trie_graph.graph.forward_from(np.node).begin());
            } else {
                new(&its.fwd) EdgeIterImplGraphFwd<Edge>(
                        node.seg[np.pos],
                        np);
            }
            ++nps;
            return true;
        }
        return false;
    }

    virtual Edge get() const {
        return base()->get();
    }

};

template <typename Handle_, typename Letter_, typename TrieGraphData_>
union EditEdgeImplHolder {
    using Handle = Handle_;
    using Letter = Letter_;
    using TrieGraphData = TrieGraphData_;
    using Graph = TrieGraphData::Graph;
    using NodePos = typename Handle::NodePos;
    using Self = EditEdgeImplHolder;
    using Edge = EditEdge<Handle, Letter>;

    EdgeIterImplBase<Edge> base;
    EdgeIterImplGraphFwd<Edge> graph_fwd;
    EdgeIterImplGraphSplit<Edge, Graph> graph_split;
    EdgeIterImplTrieInner<Edge> trie_inner;
    EdgeIterImplTrieToGraph<Edge, TrieGraphData> trie_to_graph;

    EditEdgeImplHolder() {}
    EditEdgeImplHolder(const Self &h) { copyFrom(h); }
    EditEdgeImplHolder(Self &&h) { copyFrom(h); }
    Self &operator= (const Self &h) { copyFrom(h); return *this; }
    Self &operator= (Self &&h) { copyFrom(h); return *this; }

    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wclass-memaccess"
    void copyFrom(const Self &h) { std::memcpy(this, &h, sizeof(Self)); }
    #pragma GCC diagnostic pop

    static Self make_base(u32 state) {
        return Self().set<&Self::base>(state);
    }
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

    Self end_holder() const {
        Self end;
        end.set<&Self::base>();
        end.base.state = end_state();
        return end;
    }

    const EdgeIterImplBase<Edge> *basep() const {
        return reinterpret_cast<const EdgeIterImplBase<Edge>*>(this);
    }

    EdgeIterImplBase<Edge> *basep() {
        return reinterpret_cast<EdgeIterImplBase<Edge>*>(this);
    }

    // forwarder interface
    Edge get() const { return basep()->get(); }
    void inc() { basep()->inc(); }
    u32 state() const { return basep()->state; }
    u32 end_state() const { return basep()->end_state(); }
};

template <typename Impl_>
struct EditEdgeIter {
    using Impl = Impl_;

    using iterator_category = std::forward_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = typename Impl::Edge;
    using reference_type = value_type;
    using Self = EditEdgeIter;

    EditEdgeIter() : impl() {}
    EditEdgeIter(const Impl &impl) : impl(impl) {}
    EditEdgeIter(Impl &&impl) : impl(std::move(impl)) {}

    reference_type operator* () const { return impl.get(); }
    Self &operator++ () { impl.inc(); return *this; }
    Self operator++ (int) { Self tmp = *this; ++(*this); return tmp; }
    bool operator== (const Self &other) const { return impl.state() == other.impl.state(); }

    Impl impl;
};

template <typename Impl_>
struct EditEdgeIterHelper {
    using Impl = Impl_;
    using Iter = EditEdgeIter<Impl>;

    EditEdgeIterHelper(Impl &&impl) : it(std::move(impl)) {}

    Iter begin() const { return it; }
    Iter end() const { return Iter(it.impl.end_holder()); }

    Iter it;
};

#endif /* __EDGE_H__ */
