#ifndef __MANAGER_H__
#define __MANAGER_H__

#include "kmer.h"
#include "str.h"
#include "rgfa_graph.h"
#include "letter_loc_data.h"
#include "trie_data.h"
#include "triegraph_data.h"
#include "triegraph_builder.h"
#include "edge.h"
#include "triegraph.h"

namespace triegraph {

template<typename Cfg>
struct Manager : Cfg {
    using Kmer = triegraph::Kmer<
        typename Cfg::Letter,
        typename Cfg::KmerHolder,
        Cfg::KmerLen,
        Cfg::on_mask>;
    using Str = triegraph::Str<
        typename Cfg::Letter,
        typename Cfg::StrHolder,
        typename Cfg::NodeLen>;
    using Graph = triegraph::RgfaGraph<
        Str,
        typename Cfg::NodeLoc,
        typename Cfg::EdgeLoc>;
    using NodePos = triegraph::NodePos<
        typename Cfg::NodeLoc,
        typename Cfg::NodeLen>;
    using LetterLocData = triegraph::LetterLocData<
        NodePos,
        Graph,
        typename Cfg::LetterLoc>;
    using TrieData = triegraph::TrieData<
        Kmer,
        typename Cfg::LetterLoc>;
    using TrieGraphData = triegraph::TrieGraphData<
        Graph,
        LetterLocData,
        TrieData>;
    using TrieGraphBuilder = triegraph::TrieGraphBuilder<
        TrieGraphData>;

    using Handle = triegraph::Handle<Kmer, NodePos>;
    using EditEdge = triegraph::EditEdge<Handle>;
    using EditEdgeIter = triegraph::EditEdgeIter<Handle, TrieGraphData>;
    using EditEdgeIterHelper = triegraph::EditEdgeIterHelper<EditEdgeIter>;

    using TrieGraph = triegraph::TrieGraph<
        TrieGraphData>;

    static TrieGraph triegraph_from_rgfa_file(const std::string &file) {
        auto graph = Graph::from_file(file);
        auto tg_data = TrieGraphBuilder(std::move(graph)).build();
        return TrieGraph(std::move(tg_data));
    }
};

} /* namespace triegraph */

#endif /* __MANAGER_H__ */
