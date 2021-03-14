#ifndef __TRIE_DATA_H__
#define __TRIE_DATA_H__

#include "hash_policy.h"

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

template <typename Key_, typename Size_, typename Val_>
struct PMMultiMap {
    using Key = Key_;
    using Size = Size_;
    using Val = Val_;
    using ValueIter = std::vector<Val>::const_iterator;
    using Map = HashPolicy<Key, Size>::Map;

    Map key2id;
    std::vector<std::vector<Val>> vals;
    Size num_vals;

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
        auto pos = key2id.find(k);
        if (pos == key2id.end()) {
            pos = key2id.insert(std::make_pair(k, vals.size())).first;
            vals.push_back({});
        }
        vals[pos->second].push_back(v);
        ++ num_vals;
    }

    bool contains(const Key &k) const {
        return key2id.contains(k);
    }

    // maybe implement keyvalues_for that returns KeyValueIter
    std::pair<ValueIter, ValueIter> values_for(const Key &k) const {
        auto pos = key2id.find(k);
        if (pos != key2id.end()) {
            return std::make_pair(vals[pos->second].cbegin(), vals[pos->second].cend());
        }
        return std::make_pair(vals[0].end(), vals[0].end());
    }
    using local_value_iterator = ValueIter;

    GlobalIter begin() const { return GlobalIter(*this, key2id.cbegin()); }
    GlobalIter end() const { return GlobalIter(*this, key2id.cend()); }
    Size size() const { return num_vals; }
    Size key_size() const { return key2id.size(); }
};

template <typename Kmer_, typename LetterLoc_,
         typename NumKmers_ = LetterLoc_>
struct TrieData {
    using Kmer = Kmer_;
    using LetterLoc = LetterLoc_;
    using NumKmers = NumKmers_;

    PMMultiMap<Kmer_, NumKmers_, LetterLoc_> trie2graph;
    PMMultiMap<LetterLoc_, NumKmers_, Kmer_> graph2trie;

    HashPolicy<Kmer>::Set active_trie;
};


} /* namespace triegraph */

#endif /* __TRIE_DATA_H__ */
