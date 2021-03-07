#ifndef __TRIE_DATA_H__
#define __TRIE_DATA_H__

#include <unordered_map>
#include <unordered_set>

template <typename Kmer_, typename LetterLoc_>
struct TrieData {
    using Kmer = Kmer_;
    using LetterLoc = LetterLoc_;

    // for each leaf k-mer -> (many) letter locations in graph
    std::unordered_multimap<Kmer, LetterLoc> trie2graph;
    // for each letter location -> (many) leaf k-mers
    std::unordered_multimap<LetterLoc, Kmer> graph2trie;

    // which non leaf-kmer have descendant leafs
    std::unordered_set<Kmer> active_trie;

    TrieData() {}
};

#endif /* __TRIE_DATA_H__ */
