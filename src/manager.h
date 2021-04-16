#ifndef __MANAGER_H__
#define __MANAGER_H__

#include "alphabet/str.h"
#include "graph/complexity_estimator.h"
#include "graph/connected_components.h"
#include "graph/letter_loc_data.h"
#include "graph/rgfa_graph.h"
#include "graph/sparse_starts.h"
#include "graph/top_order.h"
#include "graph/complexity_component.h"
#include "graph/complexity_component_walker.h"
#include "triegraph/handle.h"
#include "triegraph/triegraph_builder_bt.h"
#include "triegraph/triegraph_builder.h"
#include "triegraph/triegraph_builder_nbfs.h"
#include "triegraph/triegraph_builder_pbfs.h"
#include "triegraph/triegraph_data.h"
#include "triegraph/triegraph_edge_iter.h"
#include "triegraph/triegraph.h"
#include "triegraph/triegraph_handle_iter.h"
#include "trie/kmer.h"
#include "trie/dkmer.h"
#include "trie/trie_data.h"
#include "trie/trie_data_opt.h"
#include "util/util.h"

namespace triegraph {

template<typename Cfg>
struct Manager : Cfg {
    static constexpr bool kmer_fixed_k = Cfg::KmerLen != 0;
    using Kmer = choose_type_t<
        kmer_fixed_k,
        triegraph::Kmer<
            typename Cfg::Letter,
            typename Cfg::KmerHolder,
            Cfg::KmerLen,
            Cfg::on_mask>,
        triegraph::DKmer<
            typename Cfg::Letter,
            typename Cfg::KmerHolder>>;
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
    using TopOrder = triegraph::TopOrder<
        Graph>;
    using ComplexityEstimator = triegraph::ComplexityEstimator<
        Graph,
        TopOrder,
        typename Cfg::KmerHolder>;
    using ComplexityComponent = triegraph::ComplexityComponent<
        Graph,
        NodePos>;
    using ComplexityComponentWalker = triegraph::ComplexityComponentWalker<
        ComplexityComponent>;
    using ConnectedComponents = triegraph::ConnectedComponents<
        Graph>;
    using SparseStarts = triegraph::SparseStarts<
        Graph,
        NodePos>;
    static_assert(!Cfg::triedata_allow_inner || Cfg::triedata_advanced);
    using TrieData = choose_type_t<
        Cfg::triedata_advanced,
        triegraph::TrieDataOpt<
            Kmer,
            LetterLocData,
            typename LetterLocData::LetterLoc,
            Cfg::triedata_allow_inner>,
        triegraph::TrieData<
            Kmer,
            typename LetterLocData::LetterLoc>>;
    using TrieGraphData = triegraph::TrieGraphData<
        Graph,
        LetterLocData,
        TrieData>;
    using TrieGraphBuilderBFS = triegraph::TrieGraphBuilder<
        Graph, LetterLocData, Kmer>;
    using TrieGraphBuilderBT = triegraph::TrieGraphBTBuilder<
        Graph, LetterLocData, Kmer>;
    using TrieGraphBuilderPBFS = triegraph::TrieGraphBuilderPBFS<
        Graph, LetterLocData, Kmer>;
    using TrieGraphBuilderNBFS = triegraph::TrieGraphBuilderNBFS<
        Graph, LetterLocData, Kmer>;

    using Handle = triegraph::Handle<Kmer, NodePos>;
    using EditEdge = triegraph::EditEdge<Handle>;

    using PrevHandleIter = triegraph::PrevHandleIter<Handle, TrieGraphData>;
    using PrevHandleIterHelper = triegraph::PrevHandleIterHelper<Handle, TrieGraphData>;

    using TrieGraph = triegraph::TrieGraph<
        TrieGraphData>;

    enum struct Algo { BFS, BACK_TRACK, POINT_BFS, NODE_BFS, UNKNOWN };
    static constexpr std::array<Algo, 4> algorithms = {
        Algo::BFS, Algo::BACK_TRACK, Algo::POINT_BFS, Algo::NODE_BFS };

    static constexpr const char *algo_name(Algo algo) {
        switch (algo) {
            case Algo::BFS: return "BFS";
            case Algo::BACK_TRACK: return "BACK_TRACK";
            case Algo::POINT_BFS: return "POINT_BFS";
            case Algo::NODE_BFS: return "NODE_BFS";
            default: return "";
        }
    }

    static constexpr Algo algo_from_name(std::string name) {
        auto lname = to_lower(name);
        if (lname == "bfs")
            return Algo::BFS;
        if (lname == "back_track" || lname == "bt")
            return Algo::BACK_TRACK;
        if (lname == "point_bfs" || lname == "pbfs")
            return Algo::POINT_BFS;
        if (lname == "node_bfs" || lname == "nbfs")
            return Algo::NODE_BFS;
        return Algo::UNKNOWN;
    }

    friend std::ostream &operator<< (std::ostream &os, const Algo &algo) {
        return os << algo_name(algo);
    }

    struct Settings {
        bool add_reverse_complement = true;
        u64 trie_depth = sizeof(typename Kmer::Holder) * BITS_PER_BYTE / Cfg::Letter::bits - 1;
        // using enum Algo;
        Algo algo;
        // enum { BFS, BACK_TRACK, POINT_BFS, NODE_BFS } algo = BFS;
        u32 skip_every = 1; // 1 means don't skip
        u32 cut_early_threshold = 0; // for POINT_BFS only

        void validate() const {
            if (algo == Algo::BFS && skip_every != 1) {
                std::cerr << "skip_every is not supported with BFS" << std::endl;
                throw -1;
            }
            if (cut_early_threshold != 0 && algo != Algo::POINT_BFS) {
                std::cerr << "cut_early_threshold is only supported for POINT_BFS" << std::endl;
                throw -1;
            }
        }


        friend std::ostream &operator<< (std::ostream &os, const Settings &s) {
            os << "add_reverse_complement=" << s.add_reverse_complement << '\n'
                << "trie_depth=" << s.trie_depth << '\n'
                << "algo=" << algo_name(s.algo) << '\n'
                << "skip_every=" << s.skip_every << '\n'
                << "cut_early_threshold=" << s.cut_early_threshold << '\n'
                << std::flush;
            return os;
        }

        struct NoSkip {};
        struct SkipEvery { u32 n; };
    };

    static TrieGraph triegraph_from_rgfa_file(const std::string &file, Settings s = {}) {
        init(s);
        return triegraph_from_graph(
                Graph::from_file(file, {
                    .add_reverse_complement = s.add_reverse_complement }),
                s);
    }

    static TrieGraph triegraph_from_graph(Graph &&graph, Settings s = {}) {
        init(s);
        if (graph.settings.add_reverse_complement != s.add_reverse_complement) {
            throw "graph was not build with same revcomp settings";
        }

        s.validate();

        using NoSkip = Settings::NoSkip;
        using SkipEvery = Settings::SkipEvery;
        if (s.skip_every == 1) {
            switch (s.algo) {
                case Algo::BFS:
                    return triegraph_from_graph_impl<TrieGraphBuilderBFS>(
                            std::move(graph), s, NoSkip {});
                case Algo::BACK_TRACK:
                    return triegraph_from_graph_impl<TrieGraphBuilderBT>(
                            std::move(graph), s, NoSkip {});
                case Algo::POINT_BFS:
                    return triegraph_from_graph_impl<TrieGraphBuilderPBFS>(
                            std::move(graph), s, NoSkip {});
                default:
                    throw 0;
            }
        } else {
            switch (s.algo) {
                case Algo::BACK_TRACK:
                    return triegraph_from_graph_impl<TrieGraphBuilderBT>(
                            std::move(graph), s, SkipEvery { s.skip_every });
                case Algo::POINT_BFS:
                    return triegraph_from_graph_impl<TrieGraphBuilderPBFS>(
                            std::move(graph), s, SkipEvery { s.skip_every });
                default:
                    throw 0;
            }
        }
    }

    using vec_pairs = std::vector<std::pair<Kmer, typename LetterLocData::LetterLoc>>;

    template<typename Builder>
    static vec_pairs pairs_from_graph(const Graph &graph, Settings s, Settings::NoSkip) {
        init(s);
        // std::cerr << "settings:\n" << s;
        auto lloc = LetterLocData(graph);
        // std::cerr << "total locations " << lloc.num_locations << std::endl;
        auto scope = Logger::get().begin_scoped("builder wrap");
        return Builder(graph, lloc).get_pairs(lloc, s.cut_early_threshold);
    }

    template <typename Builder>
    static vec_pairs pairs_from_graph(const Graph &graph,
            Settings s, Settings::SkipEvery skip_every) {
        init(s);
        // std::cerr << "settings:\n" << s;
        auto lloc = LetterLocData(graph);
        auto sp = ConnectedComponents(graph).compute_starting_points();
        auto ss = SparseStarts(graph);
        auto lsp = ss.compute_starts_every(skip_every.n, sp);
        { auto _ = std::move(sp); }
        auto scope = Logger::get().begin_scoped("builder wrap");
        return Builder(graph, lloc).get_pairs(lsp, s.cut_early_threshold);
    }

    template<typename Builder>
    static TrieGraph triegraph_from_graph_impl(Graph &&graph, Settings s, Settings::NoSkip) {
        auto lloc = LetterLocData(graph);
        auto pairs = Builder(graph, lloc).get_pairs(lloc, s.cut_early_threshold);
        auto td = TrieData(std::move(pairs), lloc);
        return TrieGraph(TrieGraphData(
                    std::move(graph), std::move(lloc), std::move(td)));
    }

    template <typename Builder>
    static TrieGraph triegraph_from_graph_impl(Graph &&graph,
            Settings s, Settings::SkipEvery skip_every) {
        auto lloc = LetterLocData(graph);
        auto sp = ConnectedComponents(graph).compute_starting_points();
        auto ss = SparseStarts(graph);
        auto lsp = ss.compute_starts_every(skip_every.n, sp);
        { auto _ = std::move(sp); }
        auto pairs = Builder(graph, lloc).get_pairs(lsp, s.cut_early_threshold);
        { auto _ = std::move(ss); }
        auto td = TrieData(std::move(pairs), lloc);
        return TrieGraph(TrieGraphData(
                    std::move(graph), std::move(lloc), std::move(td)));
    }

    struct FixedKKmer {}; struct DynamicKKmer {};
    using KmerTag = choose_type_t<kmer_fixed_k, FixedKKmer, DynamicKKmer>;

    static void init(Settings s = {}) {
        init(s, KmerTag {});
    }

    static void init(Settings s, FixedKKmer) {
        if (s.trie_depth != Kmer::K) {
            throw "trie depth doesn't match Kmer::K";
        }
    }

    static void init(Settings s, DynamicKKmer) {
        using Holder = Kmer::Holder;
        auto on_mask = Holder(1) << (sizeof(Holder) * BITS_PER_BYTE - 1);
        Logger::get().log("setting",
                "trie_depth =", s.trie_depth,
                "on_mask =", std::hex, on_mask, std::dec);
        Kmer::setK(s.trie_depth, on_mask);
    }
};

} /* namespace triegraph */

#endif /* __MANAGER_H__ */
