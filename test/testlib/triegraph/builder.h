// SPDX-License-Identifier: MPL-2.0
/*
 * Copyright (c) 2021, Iskren Chernev
 */

#ifndef __TESTLIB_TRIEGRAPH_BUILDER_H__
#define __TESTLIB_TRIEGRAPH_BUILDER_H__

#include <util/util.h>
#include <trie/kmer_settings.h>

namespace test {

template <typename TG>
static TG::TrieGraph tg_from_graph(typename TG::Graph &&g, triegraph::u32 trie_depth) {
    auto lloc = typename TG::LetterLocData(g);
    auto pairs = TG::template graph_to_pairs<typename TG::TrieBuilderNBFS>(
            g,
            lloc,
            triegraph::KmerSettings::from_depth<typename TG::KmerHolder>(trie_depth),
            {},
            lloc);
    auto td = TG::pairs_to_triedata(std::move(pairs), lloc);
    return TG::triedata_to_triegraph(std::move(td), std::move(g), std::move(lloc));
}

} /* namespace test */

#endif /* __TESTLIB_TRIEGRAPH_BUILDER_H__ */
