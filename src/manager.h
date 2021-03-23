#ifndef __MANAGER_H__
#define __MANAGER_H__

#include "alphabet/str.h"
#include "graph/letter_loc_data.h"
#include "graph/rgfa_graph.h"
#include "triegraph/handle.h"
#include "triegraph/triegraph_builder_bt.h"
#include "triegraph/triegraph_builder.h"
#include "triegraph/triegraph_data.h"
#include "triegraph/triegraph_edge_iter.h"
#include "triegraph/triegraph.h"
#include "triegraph/triegraph_handle_iter.h"
#include "trie/kmer.h"
#include "trie/trie_data.h"
#include "trie/trie_data_opt.h"

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
        typename Cfg::LetterLoc,
        Cfg::LetterLocIdxShift>;
    using TrieData = triegraph::TrieDataOpt<
        Kmer,
        typename Cfg::LetterLoc>;
    using TrieGraphData = triegraph::TrieGraphData<
        Graph,
        LetterLocData,
        TrieData>;
    using TrieGraphBuilder = triegraph::TrieGraphBuilder<
        TrieGraphData>;
    using TrieGraphBTBuilder = triegraph::TrieGraphBTBuilder<
        TrieGraphData>;

    using Handle = triegraph::Handle<Kmer, NodePos>;
    using EditEdge = triegraph::EditEdge<Handle>;

    using PrevHandleIter = triegraph::PrevHandleIter<Handle, TrieGraphData>;
    using PrevHandleIterHelper = triegraph::PrevHandleIterHelper<Handle, TrieGraphData>;

    using TrieGraph = triegraph::TrieGraph<
        TrieGraphData>;

    struct Settings {
        bool add_reverse_complement = true;
        u64 trie_depth = Kmer::K;
    };

    template<typename Builder=TrieGraphBuilder>
    static TrieGraph triegraph_from_rgfa_file(const std::string &file, Settings s = {}) {
        return triegraph_from_graph<Builder>(
                Graph::from_file(file, {
                    .add_reverse_complement = s.add_reverse_complement }),
                s);
    }

    template<typename Builder=TrieGraphBuilder>
    static TrieGraph triegraph_from_graph(Graph &&graph, Settings s = {}) {
        if (graph.settings.add_reverse_complement != s.add_reverse_complement) {
            throw "graph was not build with same revcomp settings";
        }
        if (s.trie_depth != Kmer::K) {
            throw "trie depth doesn't match Kmer::K";
        }
        auto tg_data = Builder(std::move(graph)).build();
        return TrieGraph(std::move(tg_data));
    }
};

} /* namespace triegraph */

#endif /* __MANAGER_H__ */
