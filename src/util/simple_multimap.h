#ifndef __SIMPLE_MULTIMAP_H__
#define __SIMPLE_MULTIMAP_H__

namespace triegraph {

template <typename K, typename V,
         typename Map = std::unordered_multimap<K, V>>
struct SimpleMultimap {
    Map map;

    SimpleMultimap() {}
    SimpleMultimap(std::ranges::input_range auto&& range) {
        for (const auto &p : range) {
            map.emplace(p.first, p.second);
        }
    }

    size_t size() const { return map.size(); }
    bool contains(const K &k) const { return map.contains(k); }

    struct PairIter {
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = std::pair<K, V>;
        using reference = value_type;
        using Self = PairIter;

        PairIter(Map::const_iterator it = {}) : it(it) {}

        reference operator* () const { return {it->first, it->second}; }
        Self &operator++ () { ++it; return *this; }
        Self operator++ (int) { Self tmp = *this; ++(*this); return tmp; }
        bool operator== (const Self &other) const { return it == other.it; }

        Map::const_iterator it;
    };
    using const_iterator = PairIter;
    const_iterator begin() const { return map.begin(); }
    const_iterator end() const { return map.end(); }
    auto equal_range(const K &key) const { return map.equal_range(key); }


    struct KeyIter {
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = K;
        using reference = value_type;
        using Self = KeyIter;

        KeyIter(const_iterator it = {}) : it(it) {}

        reference operator* () const { return (*it).first; }
        Self &operator++ () { ++it; return *this; }
        Self operator++ (int) { Self tmp = *this; ++(*this); return tmp; }
        bool operator== (const Self &other) const { return it == other.it; }

        const_iterator it;
    };

    using const_key_iterator = KeyIter;
    iter_pair<const_key_iterator, const_key_iterator> keys() const {
        return { const_key_iterator(map.begin()), const_key_iterator(map.end()) };
    }

    struct ValIter {
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = V;
        using reference = value_type;
        using Self = ValIter;

        ValIter(const_iterator it = {}) : it(it) {}

        reference operator* () const { return (*it).second; }
        Self &operator++ () { ++it; return *this; }
        Self operator++ (int) { Self tmp = *this; ++(*this); return tmp; }
        bool operator== (const Self &other) const { return it == other.it; }

        const_iterator it;
    };

    using const_value_iterator = ValIter;
    iter_pair<const_value_iterator, const_value_iterator> values_for(const K &key) const {
        auto res = map.equal_range(key);
        return { const_value_iterator(res.first), const_value_iterator(res.second) };
    }
};

} /* namespace triegraph */

#endif /* __SIMPLE_MULTIMAP_H__ */
