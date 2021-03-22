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
    using EditEdgeIter = triegraph::EditEdgeIter<Handle, TrieGraphData>;
    using EditEdgeIterHelper = triegraph::EditEdgeIterHelper<EditEdgeIter>;

    using PrevHandleIter = triegraph::PrevHandleIter<Handle, TrieGraphData>;
    using PrevHandleIterHelper = triegraph::PrevHandleIterHelper<Handle, TrieGraphData>;

    using TrieGraph = triegraph::TrieGraph<
        TrieGraphData>;

    struct Settings {
        using Self = Settings;
        bool add_reverse_complement_;
        u64 trie_depth_;

        Settings() : add_reverse_complement_(true), trie_depth_(Kmer::K) {}
        bool add_reverse_complement() const { return add_reverse_complement_; }
        Self &add_reverse_complement(bool val) { add_reverse_complement_ = val; return *this; }
        u64 trie_depth() const { return trie_depth_; }
        Self &trie_depth(u64 val) { trie_depth_ = val; return *this; }
    };

    template<typename Builder=TrieGraphBuilder>
    static TrieGraph triegraph_from_rgfa_file(const std::string &file, Settings s) {
        return triegraph_from_graph<Builder>(Graph::from_file(file), s);
    }

    template<typename Builder=TrieGraphBuilder>
    static TrieGraph triegraph_from_graph(Graph &&graph, Settings s) {
        if (s.add_reverse_complement()) {
            graph.add_reverse_complement();
        }
        if (s.trie_depth() != Kmer::K) {
            throw "trie depth doesn't match Kmer::K";
        }
        auto tg_data = Builder(std::move(graph)).build();
        return TrieGraph(std::move(tg_data));
    }
};

} /* namespace triegraph */

#endif /* __MANAGER_H__ */
