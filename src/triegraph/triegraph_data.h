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

    Graph graph;
    LetterLocData letter_loc;
    TrieData trie_data;

    TrieGraphData(Graph &&g, LetterLocData &&lloc, TrieData &&trie_data)
      : graph(std::move(g)),
        letter_loc(std::move(lloc)),
        trie_data(std::move(trie_data)) {
    }

    TrieGraphData(const TrieGraphData &) = delete;
    TrieGraphData &operator= (const TrieGraphData &) = delete;
    TrieGraphData(TrieGraphData &&) = default;
    TrieGraphData &operator= (TrieGraphData &&) = default;
};

} /* triegraph */

#endif /* __TRIEGRAPH_DATA_H__ */
