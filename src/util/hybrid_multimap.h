#ifndef __HYBRID_MULTIMAP_H__
#define __HYBRID_MULTIMAP_H__

#include "util/util.h"

#include <utility>
#include <vector>

namespace triegraph {

template <typename Key_, typename ElemNum_, typename Val_, typename Map_>
requires (std::is_same_v<typename Map_::key_type, Key_> &&
        std::is_same_v<typename Map_::mapped_type, std::pair<ElemNum_, ElemNum_>>)
struct HybridMultimap {
    using Map = Map_;
    using Key = Key_;
    using Val = Val_;
    using ElemNum = ElemNum_;

    Map map;
    std::vector<Val_> elems;

    HybridMultimap() {}
    HybridMultimap(std::ranges::sized_range auto&& range) {
        if (range.size() == 0) {
            return;
        }

        elems.reserve(range.size());

        auto it_beg = range.begin();
        auto it_end = range.end();

        Key k = (*it_beg).first;
        ElemNum beg = elems.size();

        elems.push_back((*it_beg).second);
        for (++it_beg; it_beg != it_end; ++it_beg) {
            if ((*it_beg).first != k) {
                map.emplace(k, std::make_pair(beg, ElemNum(elems.size())));
                k = (*it_beg).first;
                beg = elems.size();
            }
            elems.push_back((*it_beg).second);
        }
        map.emplace(k, std::make_pair(beg, ElemNum(elems.size())));
    }

    struct PairIter {
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = std::pair<Key, Val>;
        using reference = value_type;

        using Self = PairIter;
        using Parent = HybridMultimap;
        using It = Map::const_iterator;

        const Parent *parent;
        It it;
        ElemNum crnt;

        PairIter() : parent(nullptr) {}
        PairIter(const Parent &p, It it = {})
            : parent(&p),
              it(it),
              crnt(it == p.map.end() ? p.elems.size() : it->second.first)
        {}
        PairIter(const Parent &p, It it, ElemNum crnt)
            : parent(&p),
              it(it),
              crnt(crnt)
        {}

        reference operator* () const { return {(*it).first, parent->elems[crnt] }; }
        Self &operator++ () {
            if (++crnt == (*it).second.second) {
                if (++it == parent->map.end())
                    crnt = parent->elems.size();
                else
                    crnt = (*it).second.first;
            }
            return *this;
        }
        Self operator++ (int) { Self tmp = *this; ++(*this); return tmp; }
        bool operator== (const Self &other) const { return it == other.it && crnt == other.crnt; }
    };

    using const_iterator = PairIter;

    const_iterator begin() const { return { *this, map.begin() }; }
    const_iterator end() const { return { *this, map.end() }; }

    struct KeyIter {
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = Key;
        using reference = value_type;

        using Self = KeyIter;
        using It = Map::const_iterator;

        It it;

        KeyIter(It it = {}) : it(it) {}

        reference operator* () const { return it->first; }
        Self &operator++ () { ++it; return *this; }
        Self operator++ (int) { Self tmp = *this; ++(*this); return tmp; }
        bool operator== (const Self &other) const { return it == other.it; }
    };

    using const_key_iterator = KeyIter;

    iter_pair<const_key_iterator, const_key_iterator> keys() const {
        return { { map.begin() }, { map.end() } };
    }

    using const_value_iterator = decltype(elems)::const_iterator;
    iter_pair<const_value_iterator, const_value_iterator> values_for(const Key &key) const {
        if (auto it = map.find(key); it != map.end())
            return { elems.begin() + it->second.first, elems.begin() + it->second.second };
        return { elems.end(), elems.end() };
    }

    std::pair<const_iterator, const_iterator> equal_range(const Key &key) const {
        if (auto it = map.find(key); it != map.end())
            return { { *this, it }, { *this, it, (*it).second.second } };
        return { { *this, map.end(), ElemNum(elems.size()) }, { *this, map.end(), ElemNum(elems.size()) } };
    }

    bool contains(const Key &key) const { return map.contains(key); }
};

} /* namespace triegraph */

#endif /* __HYBRID_MULTIMAP_H__ */
