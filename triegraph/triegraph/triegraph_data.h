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

    TrieGraphData(const Graph &g, const LetterLocData &lloc, const TrieData &trie_data)
      : graph_(&g),
        letter_loc_(&lloc),
        trie_data_(&trie_data)
    {}

    TrieGraphData(Graph &&g, LetterLocData &&lloc, TrieData &&trie_data)
      : graph_h(std::move(g)),
        letter_loc_h(std::move(lloc)),
        trie_data_h(std::move(trie_data)),
        graph_(&graph_h.value()),
        letter_loc_(&letter_loc_h.value()),
        trie_data_(&trie_data_h.value())
    {}

    TrieGraphData(const TrieGraphData &) = delete;
    TrieGraphData(TrieGraphData &&other)
        : graph_h(std::move(other.graph_h)),
          letter_loc_h(std::move(other.letter_loc_h)),
          trie_data_h(std::move(other.trie_data_h)),
          graph_(graph_h ? &graph_h.value() : other.graph_),
          letter_loc_(letter_loc_h ? &letter_loc_h.value() : other.letter_loc_),
          trie_data_(trie_data_h ? &trie_data_h.value() : other.trie_data_)
    {}
    TrieGraphData &operator= (const TrieGraphData &) = delete;
    TrieGraphData &operator= (TrieGraphData &&) = delete;

    const Graph &graph() const { return *graph_; }
    const LetterLocData &letter_loc() const { return *letter_loc_; }
    const TrieData &trie_data() const { return *trie_data_; }

private:
    // Support both moved and referenced values
    std::optional<Graph> graph_h;
    std::optional<LetterLocData> letter_loc_h;
    std::optional<TrieData> trie_data_h;

    const Graph *graph_;
    const LetterLocData *letter_loc_;
    const TrieData *trie_data_;
};

} /* triegraph */

#endif /* __TRIEGRAPH_DATA_H__ */
