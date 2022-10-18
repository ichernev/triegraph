// SPDX-License-Identifier: MPL-2.0
/*
 * Copyright (c) 2021, Iskren Chernev
 */

#ifndef __TRIEGRAPH_DATA_H__
#define __TRIEGRAPH_DATA_H__

namespace triegraph {

template<typename Graph_, typename LetterLocData_, typename TrieData_>
struct TrieGraphData {
    using Graph = Graph_;
    using LetterLocData = LetterLocData_;
    using TrieData = TrieData_;

    const Graph &graph;
    const LetterLocData &letter_loc;
    const TrieData &trie_data;

    TrieGraphData(const Graph &g, const LetterLocData &lloc, const TrieData &trie_data)
      : graph(g),
        letter_loc(lloc),
        trie_data(trie_data) {
    }

    TrieGraphData(const TrieGraphData &) = default;
    TrieGraphData &operator= (const TrieGraphData &) = delete;
    TrieGraphData(TrieGraphData &&) = default;
    TrieGraphData &operator= (TrieGraphData &&) = delete;
};

} /* triegraph */

#endif /* __TRIEGRAPH_DATA_H__ */
