#ifndef __TRIEGRAPH_HANDLE_ITER_H__
#define __TRIEGRAPH_HANDLE_ITER_H__

#include <cstring> /* memcpy */
#include <utility> /* std::forward */
#include <iterator> /* std::input_iterator_tag */

namespace triegraph {

template <typename Handle_>
struct PrevHandleIterBase {
    using Handle = Handle_;
    using Self = PrevHandleIterBase;

    virtual Handle get() const { return Handle{}; }
    virtual void inc() {}
    virtual bool has_more() const { return false; }
};

template <typename Handle_>
struct PrevHandleIterLinear final : PrevHandleIterBase<Handle_> {
    using Handle = Handle_;

    Handle h;
    PrevHandleIterLinear(Handle h = {}) : h(h) {}

    virtual Handle get() const { return h; }
    virtual void inc() { h = Handle(); }
    virtual bool has_more() const { return h.is_valid(); }
};

template <typename Handle_, typename Graph_>
struct PrevHandleIterSplit final : PrevHandleIterBase<Handle_> {
    using Handle = Handle_;
    using Graph = Graph_;
    using GraphIter = Graph::const_iterator;

    GraphIter it;
    PrevHandleIterSplit(GraphIter it = {}) : it(it) {}

    virtual Handle get() const { auto to = *it; return { to.node_id, to.seg.size() - 1 }; }
    virtual void inc() { ++it; }
    virtual bool has_more() const { return it.has_more(); }
};

template <typename Handle_, typename Graph_>
struct PrevHandleIter {
    using Handle = Handle_;
    using Graph = Graph_;
    using Self = PrevHandleIter;

    union {
        PrevHandleIterBase<Handle> base;
        PrevHandleIterLinear<Handle> linear;
        PrevHandleIterSplit<Handle, Graph> split;
    };

    PrevHandleIter() : base() {}
    PrevHandleIter(const PrevHandleIter &other) { copyFrom(other); }
    PrevHandleIter(PrevHandleIter &&other) { copyFrom(other); }
    Self &operator= (const PrevHandleIter &other) { copyFrom(other); }
    Self &operator= (PrevHandleIter &&other) { copyFrom(other); }

    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wclass-memaccess"
    void copyFrom(const Self &other) { std::memcpy(this, &other, sizeof(Self)); }
    #pragma GCC diagnostic pop

    // proxy interface

    using IBase = PrevHandleIterBase<Handle>;
    const IBase *basep() const { return reinterpret_cast<const IBase*>(this); }
    IBase *basep() { return reinterpret_cast<IBase*>(this); }

    Handle get() const { return basep()->get(); }
    void inc() { basep()->inc(); }
    bool has_more() const { return basep()->has_more(); }

    // Iterator interface

    using iterator_category = std::input_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = Handle;
    using reference_type = value_type;

    reference_type operator* () const { return get(); }
    Self &operator++ () { inc(); return *this; }
    Self operator++ (int) { Self tmp = *this; ++(*this); return tmp; }
    // TODO: This is obviously a hack, but it should work
    bool operator== (const Self &other) const { return has_more() == other.has_more(); }

    // factory
    static Self make(const Graph &g, Handle h) {
        if (h.pos() == 0) {
            return make_split(g.backward_from(h.node()).begin());
        } else {
            return make_linear(Handle(h.node(), h.pos() - 1));
        }
    }

    template <typename... Args>
    static Self make_linear(Args&&... args) {
        return Self().set<&Self::linear>(std::forward<Args>(args)...);
    }
    template <typename... Args>
    static Self make_split(Args&&... args) {
        return Self().set<&Self::split>(std::forward<Args>(args)...);
    }
    template <auto Self::*field, typename... Args>
    Self& set(Args&&... args) {
        new(&(this->*field)) std::decay<decltype(this->*field)>::type(std::forward<Args>(args)...);
        return *this;
    }
};

template <typename Handle_, typename Graph_>
struct PrevHandleIterHelper {
    using Handle = Handle_;
    using Graph = Graph_;
    using Iter = PrevHandleIter<Handle, Graph>;

    PrevHandleIterHelper(const Iter &it) : it(it) {}
    PrevHandleIterHelper(Iter &&it) : it(std::move(it)) {}

    Iter begin() const { return it; }
    Iter end() const { return Iter(); }

    Iter it;
};

} /* namespace triegraph */

#endif /* __TRIEGRAPH_HANDLE_ITER_H__ */
