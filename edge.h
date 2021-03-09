#ifndef __EDGE_H__
#define __EDGE_H__

#include <string.h> /* memcpy */
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

template <typename Handle_, typename Letter_>
struct EdgeIterImplBase {
    using Handle = Handle_;
    using Letter = Letter_;
    using Edge = EditEdge<Handle, Letter>;
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
template <typename Handle_, typename Letter_>
struct EdgeIterImplGraphFwd : EdgeIterImplBase<Handle_, Letter_> {
    using Handle = Handle_;
    using Letter = Letter_;
    using Edge = EditEdge<Handle, Letter>;

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

template <typename Handle_, typename Letter_, typename Graph_>
struct EdgeIterImplGraphSplit : EdgeIterImplBase<Handle_, Letter_> {
    using Handle = Handle_;
    using Letter = Letter_;
    using Edge = EditEdge<Handle, Letter>;

    using Graph = Graph_;

    Letter actual;
    typename Handle::NodePos p0;
    typename Graph::const_iterator it;

    EdgeIterImplGraphSplit(Letter actual, typename Handle::NodePos p0,
            typename Graph::const_iterator it)
        : actual(actual), p0(p0), it(it) {
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

template <typename Handle_, typename Letter_, typename Graph_>
union EditEdgeImplHolder {
    using H = Handle_;
    using L = Letter_;
    using G = Graph_;
    using NP = typename H::NodePos;
    using Self = EditEdgeImplHolder;
    using Edge = EditEdge<H, L>;

    EdgeIterImplBase<H, L> base;
    EdgeIterImplGraphFwd<H, L> graph_fwd;
    EdgeIterImplGraphSplit<H, L, G> graph_split;

    EditEdgeImplHolder() {}
    EditEdgeImplHolder(const Self &h) { copyFrom(h); }
    EditEdgeImplHolder(Self &&h) { copyFrom(h); }
    Self &operator= (const Self &h) { copyFrom(h); return *this; }
    Self &operator= (Self &&h) { copyFrom(h); return *this; }

    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wclass-memaccess"
    void copyFrom(const Self &h) { memcpy(this, &h, sizeof(Self)); }
    #pragma GCC diagnostic pop

    // template <typename T, T Self::*field, typename... Args>
    // static Self make(Args&&... args) {
    //     return Self().set<T, field>(std::forward(args)...);
    // }
    static Self make_base(u32 state) {
        return Self().set<EdgeIterImplBase<H, L>, &Self::base>(state);
    }
    static Self make_graph_fwd(L l, NP np) {
        return Self().set<EdgeIterImplGraphFwd<H, L>, &Self::graph_fwd>(l, np);
    }
    static Self make_graph_split(L l, NP np, typename G::const_iterator it) {
        return Self().set<EdgeIterImplGraphSplit<H, L, G>, &Self::graph_split>(l, np, it);
    }

    template <typename T, T Self::*field, typename... Args>
    Self& set(Args&&... args) {
        new(&(this->*field)) T(std::forward<Args>(args)...);
        return *this;
    }

    Self end_holder() const {
        Self end;
        end.set<EdgeIterImplBase<H, L>, &Self::base>();
        end.base.state = end_state();
        return end;
    }

    const EdgeIterImplBase<H, L> *basep() const {
        return reinterpret_cast<const EdgeIterImplBase<H, L>*>(this);
    }

    EdgeIterImplBase<H, L> *basep() {
        return reinterpret_cast<EdgeIterImplBase<H, L>*>(this);
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
