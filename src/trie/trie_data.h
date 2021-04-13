#ifndef __TRIE_DATA_H__
#define __TRIE_DATA_H__

#include "trie/hash_policy.h"

#include <utility>
// #include <unordered_map>
// #include <unordered_set>

namespace triegraph {

// template <typename Kmer_, typename LetterLoc_>
// struct TrieData {
//     using Kmer = Kmer_;
//     using LetterLoc = LetterLoc_;

//     // for each leaf k-mer -> (many) letter locations in graph
//     std::unordered_multimap<Kmer, LetterLoc> trie2graph;
//     // for each letter location -> (many) leaf k-mers
//     std::unordered_multimap<LetterLoc, Kmer> graph2trie;

//     // which non leaf-kmer have descendant leafs
//     std::unordered_set<Kmer> active_trie;

//     TrieData() {}
// };
template <typename Key_, typename Val_>
struct SimpleMMap {
    using Key = Key_;
    using Val = Val_;
    // using ValueIter = std::vector<Val>::const_iterator;
    using Map = std::unordered_multimap<Key, Val>;

    Map key_vals;

    using const_iterator = Map::const_iterator;
    struct KeyPair {
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = Key;
        using reference_type = value_type;
        using Self = KeyPair;

        KeyPair(const const_iterator &begin, const const_iterator &end)
            : it(begin), end_(end) {}
        // KeyIter() : p(nullptr), a() {}
        // KeyIter(const Parent &p) : p(&p), a(p.start.size()) {}
        // KeyIter(const Parent &p, A a) : p(&p), a(a) { adjust(); }
        const_iterator begin() const { return it; }
        const_iterator end() const { return end_; }

        reference_type operator* () const { return it->first; }
        Self &operator++ () { adjust(it->first); return *this; }
        Self operator++ (int) { Self tmp = *this; ++(*this); return tmp; }
        bool empty() const { return it == end_; }
        // bool operator== (const Self &other) const { return it == other.it; }

        void adjust(const Key &key) {
            while (++it != end && it->first == key)
                ;
        }

        const_iterator it, end_;
    };

    struct ValueIter {
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = Val;
        using reference_type = value_type;
        using Self = ValueIter;

        ValueIter(const const_iterator &it) : it(it) {}

        reference_type operator* () const { return it->second; }
        Self &operator++ () { ++it; return *this; }
        Self operator++ (int) { Self tmp = *this; ++(*this); return tmp; }
        bool operator== (const Self &other) const { return it == other.it; }

        const_iterator it;
    };
    using const_value_iterator = ValueIter;

    const_iterator begin() const { return key_vals.begin(); }
    const_iterator end() const { return key_vals.end(); }

    KeyPair keys() const {
        return KeyPair(begin(), end());
    }

    iter_pair<const_value_iterator, const_value_iterator> values_for(const Key &key) const {
        return { const_value_iterator(begin()), const_value_iterator(end()) };
    }

    std::pair<const_iterator, const_iterator> equal_range(const Key &key) const {
        return key_vals.equal_range(key);
    }

    bool contains(const Key &key) const {
        return key_vals.contains(key);
    }
};

template <typename Key_, typename Size_, typename Val_>
struct PMMultiMap {
    using Key = Key_;
    using Size = Size_;
    using Val = Val_;
    using ValueIter = std::vector<Val>::const_iterator;
    using Map = std::unordered_map<Key, Size>;

    // std::unordered_multimap<Key, Val> key2id;
    Map key2id;
    std::vector<std::vector<Val>> vals;
    Size num_vals;

    void reserve(Size sz) {
        key2id.reserve(sz);
    }

    struct GlobalIter {
        using MapIter = Map::const_iterator;
        using VecIter = std::vector<Val>::const_iterator;
        using Self = GlobalIter;

        const PMMultiMap &parent;
        MapIter map_it;
        VecIter crnt, end;

        GlobalIter(const PMMultiMap &parent, MapIter map_it) : parent(parent), map_it(map_it) {
            if (map_it != parent.key2id.cend()) {
                crnt = parent.vals[map_it->second].cbegin();
                end = parent.vals[map_it->second].cend();
            }
        }

        using iterator_category = std::input_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = std::pair<Key, Val>;
        using reference_type = value_type;

        reference_type operator* () const { return std::make_pair(map_it->first, *crnt); }
        Self &operator++ () {
            if (++crnt == end) {
                if (++map_it != parent.key2id.cend()) {
                    crnt = parent.vals[map_it->second].cbegin();
                    end = parent.vals[map_it->second].cend();
                }
            }
            return *this;
        }
        Self operator++ (int) { Self tmp = *this; ++(*this); return tmp; }
        bool operator== (const Self &other) const {
            /* not accurate, but should do the trick for end-of-loop */
            return map_it == other.map_it;
        }
    };
    using const_iterator = GlobalIter;

    void add(const Key &k, const Val &v) {
        key2id.emplace(k, v);
    }
    // void add(const Key &k, const Val &v) {
    //     auto pos = key2id.find(k);
    //     if (pos == key2id.end()) {
    //         pos = key2id.insert(std::make_pair(k, vals.size())).first;
    //         vals.push_back({});
    //     }
    //     vals[pos->second].push_back(v);
    //     ++ num_vals;
    // }

    bool contains(const Key &k) const {
        return key2id.contains(k);
    }

    // maybe implement keyvalues_for that returns KeyValueIter
    // std::pair<ValueIter, ValueIter> values_for(const Key &k) const {
    //     auto pos = key2id.find(k);
    //     if (pos != key2id.end()) {
    //         return std::make_pair(vals[pos->second].cbegin(), vals[pos->second].cend());
    //     }
    //     return std::make_pair(vals[0].end(), vals[0].end());
    // }
    // using local_value_iterator = ValueIter;

    // GlobalIter begin() const { return GlobalIter(*this, key2id.cbegin()); }
    // GlobalIter end() const { return GlobalIter(*this, key2id.cend()); }
    Size size() const { return key2id.size(); }
    Size key_size() const { return key2id.size(); }
};

template <typename Kmer_, typename LetterLoc_,
         typename NumKmers_ = LetterLoc_>
struct TrieData {
    using Kmer = Kmer_;
    using LetterLoc = LetterLoc_;
    using NumKmers = NumKmers_;
private:
    PMMultiMap<Kmer_, NumKmers_, LetterLoc_> trie2graph;
    PMMultiMap<LetterLoc_, NumKmers_, Kmer_> graph2trie;
    HashPolicy<Kmer>::Set active_trie;
public:
};


} /* namespace triegraph */

#endif /* __TRIE_DATA_H__ */
