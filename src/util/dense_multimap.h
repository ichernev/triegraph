#ifndef __DENSE_MULTIMAP_H__
#define __DENSE_MULTIMAP_H__

#include "util/multimaps.h"

#include <vector>
#include <ranges>
#include <assert.h>

namespace triegraph {

template <typename A, typename B, typename C,
         typename StartsContainer_ = std::vector<B>>
struct DenseMultimap {
    using StartsContainer = StartsContainer_;

    static constexpr auto impl = MultimapImpl::DENSE;
    // TODO(opt): compute sizes for sorted-vector from histograms

    DenseMultimap() {}
    DenseMultimap(StartsContainer &&starts, std::vector<C> &&elems)
        : starts(std::move(starts)),
          elems(std::move(elems))
    {}
    DenseMultimap(std::ranges::sized_range auto&& range) {
        // this->max_a = a_max;
        auto last = range.end(); --last;
        A a_max = (*last).first + 1;

        starts.reserve(a_max);
        elems.reserve(range.size());
        // end.resize(pairs.size(), false);
        // std::cerr << "pushing..." << std::endl;
        auto cp = range.begin(), ep = range.end();
        for (A a = 0; a < a_max; ++a) {
            // std::cerr << "now at a " << a << std::endl;
            // std::cerr << Accessor::getA(*cp) << " "
            //     <<  cookA(Accessor::getA(*cp)) << std::endl;
            starts.push_back(elems.size());
            if (cp == ep || (*cp).first != a) {
                continue;
            }
            // end[elems.size()] = true;
            while (cp != ep && (*cp).first == a) {
                elems.push_back((*cp).second);
                ++cp;
            }
        }

        // starts.sanity_check();
    }
    void sanity_check() {
        // assert(start.size() == this->max_a);
        // assert(elems.size() == range.size());
        for (A a = 1; a < starts.size(); ++a) {
            assert(starts[a-1] <= starts[a]);
        }
    }

    DenseMultimap(const DenseMultimap &) = delete;
    DenseMultimap(DenseMultimap &&) = default;
    DenseMultimap &operator= (const DenseMultimap &) = delete;
    DenseMultimap &operator= (DenseMultimap &&) = default;

    size_t size() const { return elems.size(); }
    // size_t key_size() const { return elems.size(); }

    struct PairIter {
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = std::pair<A, C>;
        using reference = value_type;

        using Self = PairIter;
        using Parent = DenseMultimap;

        PairIter() : p(nullptr), a(), b() {}
        PairIter(const Parent &p) : p(&p), a(p.starts.size()), b(p.elems.size()) {}
        PairIter(const Parent &p, A a)
            : p(&p), a(a), b(p.start(a))
        {
            adjust();
        }
        reference operator* () const {
            // std::cerr << "IN **" << a << " " << b << " " << p->elems[b] << std::endl;
            return {a, p->elems.at(b)};
        }
        Self &operator++ () {
            // std::cerr << "a = " << a << " " << p->bound(a) << std::endl;
            if (++b == p->start(a+1))
                adjust();
            return *this;
        }
        Self operator++ (int) { Self tmp = *this; ++(*this); return tmp; }
        bool operator== (const Self &other) const { return b == other.b; }
        difference_type operator- (const Self &other) const { return b - other.b; }

    private:
        void adjust() {
            while (a < p->starts.size() && p->start(a+1) <= b)
                ++a;
        }

        const Parent *p;
        A a;
        B b;
    };

    struct KeyIter {
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = A;
        using reference = value_type;
        using Self = KeyIter;
        using Parent = DenseMultimap;

        KeyIter() : p(nullptr), a() {}
        KeyIter(const Parent &p) : p(&p), a(p.starts.size()) {}
        KeyIter(const Parent &p, A a) : p(&p), a(a) { adjust(); }

        reference operator* () const { return a; }
        Self &operator++ () { ++a; adjust(); return *this; }
        Self operator++ (int) { Self tmp = *this; ++(*this); return tmp; }
        bool operator== (const Self &other) const { return a == other.a; }

        void adjust() {
            // skip "empty" keys
            while (a < p->starts.size() && p->start(a) == p->start(a+1))
                ++a;
        }

        const Parent *p;
        A a;
    };

    struct ValIter {
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = C;
        using reference = value_type;
        using Self = ValIter;

        ValIter() : it() {}
        ValIter(std::vector<value_type>::const_iterator it) : it(it) {}

        reference operator* () const { return *it; }
        Self &operator++ () { ++it; return *this; }
        Self operator++ (int) { Self tmp = *this; ++(*this); return tmp; }
        difference_type operator- (const Self &other) { return it - other.it; }
        bool operator== (const Self &other) const { return it == other.it; }

        std::vector<value_type>::const_iterator it;
    };

    using const_iterator = PairIter;
    using const_key_iterator = KeyIter;
    using const_value_iterator = ValIter;

    const_iterator begin() const { return const_iterator(*this, 0); }
    const_iterator end() const { return const_iterator(*this); }
    iter_pair<const_key_iterator, const_key_iterator> keys() const {
        return { const_key_iterator(*this, 0), const_key_iterator(*this) };
    }
    iter_pair<const_value_iterator, const_value_iterator> values_for(const A &a) const {
        return { elems.begin() + start(a), elems.begin() + start(a+1) };
    }

    std::pair<const_iterator, const_iterator> equal_range(const A &a) const {
        // for k=16 a+1 might overflow
        return std::make_pair(const_iterator(*this, a), const_iterator(*this, a+1));
    }

    bool contains(const A &a) const { return start(a) < start(a+1); }

private:
    B start(A a) const {
        if (a >= starts.size())
            return elems.size();
        return starts.at(a);
    }

    StartsContainer starts; // indexed by A
    std::vector<C> elems; // indexed by B
};

} /* namespace triegraph */

#endif /* __DENSE_MULTIMAP_H__ */
