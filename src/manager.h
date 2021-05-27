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
#include "trie/kmer_settings.h"
#include "trie/builder/bt.h"
#include "trie/builder/lbfs.h"
#include "trie/builder/nbfs.h"
#include "trie/builder/pbfs.h"
#include "triegraph/triegraph_data.h"
#include "triegraph/triegraph_edge_iter.h"
#include "triegraph/triegraph.h"
#include "triegraph/triegraph_handle_iter.h"
#include "trie/kmer.h"
#include "trie/dkmer.h"
#include "trie/trie_data.h"
#include "util/compact_vector.h"
#include "util/dense_multimap.h"
#include "util/hybrid_multimap.h"
#include "util/simple_multimap.h"
#include "util/sorted_vector.h"
#include "util/util.h"
#include "util/vector_pairs.h"
#include "util/vector_pairs_inserter.h"

#include <type_traits>
#include <vector>

namespace triegraph {

template<typename Cfg>
struct Manager : Cfg {
    static constexpr bool kmer_fixed_k = Cfg::KmerLen != 0;
    using Kmer = std::conditional_t<
        kmer_fixed_k,
        triegraph::Kmer<
            typename Cfg::Letter,
            typename Cfg::KmerHolder,
            Cfg::KmerLen,
            Cfg::on_mask>,
        triegraph::DKmer<
            typename Cfg::Letter,
            typename Cfg::KmerHolder>>;
    using KmerSettings = triegraph::KmerSettings;
    using KmerCodec = triegraph::KmerCodec<
        Kmer,
        typename Kmer::Holder,
        Cfg::triedata_allow_inner>;
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
    using T2GSMM = SimpleMultimap<
        typename Cfg::KmerHolder,
        typename Cfg::LetterLoc>;
    using G2TSMM = SimpleMultimap<
        typename Cfg::LetterLoc,
        typename Cfg::KmerHolder>;
    using StartsContainer = std::conditional_t<
        Cfg::triedata_sorted_vector,
        SortedVector<typename Cfg::KmerHolder>,
        std::vector<typename Cfg::KmerHolder>>;
    using T2GDMM = DenseMultimap<
        typename Cfg::KmerHolder,
        typename Cfg::LetterLoc,
        StartsContainer,
        std::conditional_t<Cfg::compactvector_for_elems,
            CompactVector<typename Cfg::LetterLoc>,
            std::vector<typename Cfg::LetterLoc> > >;
    using G2TDMM = DenseMultimap<
        typename Cfg::LetterLoc,
        typename Cfg::KmerHolder,
        StartsContainer,
        std::conditional_t<Cfg::compactvector_for_elems,
            CompactVector<typename Cfg::KmerHolder>,
            std::vector<typename Cfg::KmerHolder> > >;
    using T2GMap = choose_type_t<Cfg::TDMapType, T2GSMM, T2GDMM>;
    using G2TMap = choose_type_t<Cfg::TDMapType, G2TSMM, G2TDMM>;
    // using VectorPairs = std::vector<std::pair<Kmer, typename LetterLocData::LetterLoc>>;
    using VPFirst_ = std::conditional_t<Cfg::trie_pairs_raw, Kmer, typename Cfg::KmerHolder>;
    using VPSecond_ = LetterLocData::LetterLoc;
    using VectorPairs = choose_type_t<
        u32(Cfg::vector_pairs_impl),
        triegraph::VectorPairsEmpty<VPFirst_, VPSecond_>,
        triegraph::VectorPairsSimple<VPFirst_, VPSecond_>,
        std::conditional_t<Cfg::compactvector_for_elems,
            triegraph::VectorPairsDual<VPFirst_, VPSecond_,
                CompactVector<VPFirst_>,
                CompactVector<VPSecond_> >,
            triegraph::VectorPairsDual<VPFirst_, VPSecond_> > >;
    static constexpr auto vp_inserter_fmap = [](auto &&k) {
        return KmerCodec::to_int(std::forward<decltype(k)>(k));
    };
    using VectorPairsInserter = triegraph::VectorPairsInserter<
        VectorPairs,
        decltype(vp_inserter_fmap),
        std::identity,
        std::pair<Kmer, typename LetterLocData::LetterLoc>>;
    using VPAlgo = std::conditional_t<
        Cfg::trie_pairs_raw,
        VectorPairs,
        VectorPairsInserter>;
    using TrieData = triegraph::TrieData<
        Kmer,
        LetterLocData,
        VectorPairs,
        Cfg::triedata_allow_inner,
        T2GMap, G2TMap,
        Cfg::triedata_zero_overhead>;
    using TrieGraphData = triegraph::TrieGraphData<
        Graph,
        LetterLocData,
        TrieData>;
    using TrieBuilderLBFS = triegraph::TrieBuilderLBFS<
        Graph, LetterLocData, Kmer, VPAlgo>;
    using TrieBuilderBT = triegraph::TrieBuilderBT<
        Graph, LetterLocData, Kmer, VPAlgo>;
    using TrieBuilderPBFS = triegraph::TrieBuilderPBFS<
        Graph, LetterLocData, Kmer, VPAlgo>;
    using TrieBuilderNBFS = triegraph::TrieBuilderNBFS<
        Graph, LetterLocData, Kmer, VPAlgo>;

    using Handle = triegraph::Handle<Kmer, NodePos>;
    using EditEdge = triegraph::EditEdge<Handle>;

    using PrevHandleIter = triegraph::PrevHandleIter<Handle, TrieGraphData>;
    using PrevHandleIterHelper = triegraph::PrevHandleIterHelper<Handle, TrieGraphData>;

    using TrieGraph = triegraph::TrieGraph<
        TrieGraphData>;

    enum struct Algo { LOCATION_BFS, BACK_TRACK, POINT_BFS, NODE_BFS, UNKNOWN };
    static constexpr std::array<Algo, 4> algorithms = {
        Algo::LOCATION_BFS, Algo::BACK_TRACK, Algo::POINT_BFS, Algo::NODE_BFS };

    static constexpr const char *algo_name(Algo algo) {
        switch (algo) {
            case Algo::LOCATION_BFS: return "LOCATION_BFS";
            case Algo::BACK_TRACK: return "BACK_TRACK";
            case Algo::POINT_BFS: return "POINT_BFS";
            case Algo::NODE_BFS: return "NODE_BFS";
            default: return "";
        }
    }

    static constexpr Algo algo_from_name(std::string name) {
        auto lname = to_lower(name);
        if (lname == "bfs")
            return Algo::LOCATION_BFS;
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

    // enum struct PairsVariant { RAW, COMPRESSED };
    struct PairsVariantRaw {};
    struct PairsVariantCompressed {};

    static VectorPairs &make_pairs_inserter(VectorPairs &pairs, PairsVariantRaw) {
        // std::cerr << "returning same" << std::endl;
        return pairs;
    }
    static auto make_pairs_inserter(
            VectorPairs &pairs, PairsVariantCompressed) {
        // std::cerr << "NOT returning same" << std::endl;
        return VectorPairsInserter(pairs, vp_inserter_fmap, {});
    }

    template <typename TrieBuilder,
             typename pairs_variant = std::conditional_t<
                 Cfg::trie_pairs_raw,
                 PairsVariantRaw,
                 PairsVariantCompressed>>
    static auto graph_to_pairs(
            const Graph &graph,
            const LetterLocData &lloc,
            KmerSettings kmer_settings,
            TrieBuilder::Settings tb_settings,
            std::ranges::input_range auto&& starts) {
        // std::cerr << "FOOOOOO" << std::endl;
        Kmer::set_settings(kmer_settings);
        auto pairs = VectorPairs {};
        _vp_set_bits(pairs, lloc);
        std::conditional_t<Cfg::trie_pairs_raw,
            VectorPairs &,
            VectorPairsInserter> pairs_inserter = make_pairs_inserter(pairs, pairs_variant {});
        // pairs.emplace_back(Kmer::empty(), 0);
        // assert(pairs.size() == 1);
        // assert(pairs_inserter.size() == 1);
        // std::cerr << "&pairs " << &pairs << " " << "&pi " << &pairs_inserter << std::endl;
        TrieBuilder(graph, lloc, pairs_inserter)
            .set_settings(std::move(tb_settings))
            .compute_pairs(std::forward<decltype(starts)>(starts));
        // std::cerr << "got pairs size " << pairs.size() << std::endl;
        _vp_check_bits(pairs);
        return pairs;
    }

    static void _vp_set_bits(VectorPairs &pairs, const LetterLocData &lloc) {
        if constexpr (VectorPairs::impl == VectorPairsImpl::DUAL) {
            // TODO: Put this in CompactVectorSettings, and add cfg override
            u32 kmer_bits = log2_ceil(TrieData::total_kmers());
            u32 lloc_bits = log2_ceil(lloc.num_locations);
            u32 bits = std::max(kmer_bits, lloc_bits) + 1;

            compact_vector_set_bits(pairs.get_v1(), bits);
            compact_vector_set_bits(pairs.get_v2(), bits);
        }
    }

    static void _vp_check_bits(VectorPairs &pairs) {
        if constexpr (VectorPairs::impl == VectorPairsImpl::DUAL) {
            u32 pair_bits = log2_ceil(pairs.size());
            if (pair_bits > compact_vector_get_bits(pairs.get_v1()) ||
                    pair_bits > compact_vector_get_bits(pairs.get_v2())) {
                // TODO: Support increasing #bits in CV
                throw "not-enough-bits-for-pairs";
            }
        }
    }

    template <typename TrieBuilder, typename pairs_variant =
        std::conditional_t<Cfg::trie_pairs_raw, PairsVariantRaw, PairsVariantCompressed> >
    static VectorPairs graph_to_pairs(
            const Graph &graph,
            const auto &cfg) {
        auto lloc = LetterLocData(graph);
        return graph_to_pairs<TrieBuilder, pairs_variant>(
                graph,
                lloc,
                KmerSettings::from_seed_config<typename Cfg::KmerHolder>(
                    lloc.num_locations, cfg),
                TrieBuilder::Settings::from_config(cfg),
                lloc);
    }

    static TrieData pairs_to_triedata(
            VectorPairs &&pairs,
            const LetterLocData &lloc) {
        return TrieData(std::move(pairs), lloc);
    }

    static TrieGraph triedata_to_triegraph(
            TrieData &&td,
            Graph &&g,
            LetterLocData &&lloc) {
        return TrieGraph(TrieGraphData(std::move(g), std::move(lloc), std::move(td)));
    }

    // struct Settings {
    //     bool add_reverse_complement = true;
    //     u64 trie_depth = sizeof(typename Kmer::Holder) * BITS_PER_BYTE / Cfg::Letter::bits - 1;
    //     // using enum Algo;
    //     Algo algo;
    //     // enum { BFS, BACK_TRACK, POINT_BFS, NODE_BFS } algo = BFS;
    //     u32 skip_every = 1; // 1 means don't skip
    //     u32 cut_early_threshold = 0; // for POINT_BFS only

    //     void validate() const {
    //         if (algo == Algo::LOCATION_BFS && skip_every != 1) {
    //             std::cerr << "skip_every is not supported with BFS" << std::endl;
    //             throw -1;
    //         }
    //         if (cut_early_threshold != 0 && algo != Algo::POINT_BFS) {
    //             std::cerr << "cut_early_threshold is only supported for POINT_BFS" << std::endl;
    //             throw -1;
    //         }
    //     }


    //     friend std::ostream &operator<< (std::ostream &os, const Settings &s) {
    //         os << "add_reverse_complement=" << s.add_reverse_complement << '\n'
    //             << "trie_depth=" << s.trie_depth << '\n'
    //             << "algo=" << algo_name(s.algo) << '\n'
    //             << "skip_every=" << s.skip_every << '\n'
    //             << "cut_early_threshold=" << s.cut_early_threshold << '\n'
    //             << std::flush;
    //         return os;
    //     }

    //     struct NoSkip {};
    //     struct SkipEvery { u32 n; };
    // };

    // static TrieGraph triegraph_from_rgfa_file(const std::string &file, Settings s = {}) {
    //     init(s);
    //     return triegraph_from_graph(
    //             Graph::from_file(file, {
    //                 .add_reverse_complement = s.add_reverse_complement }),
    //             s);
    // }

    // static TrieGraph triegraph_from_graph(Graph &&graph, Settings s = {}) {
    //     init(s);
    //     if (graph.settings.add_reverse_complement != s.add_reverse_complement) {
    //         throw "graph was not build with same revcomp settings";
    //     }

    //     s.validate();

    //     using NoSkip = Settings::NoSkip;
    //     // using SkipEvery = Settings::SkipEvery;
    //     if (s.skip_every == 1) {
    //         switch (s.algo) {
    //             // case Algo::BFS:
    //             //     return triegraph_from_graph_impl<TrieGraphBuilderBFS>(
    //             //             std::move(graph), s, NoSkip {});
    //             // case Algo::BACK_TRACK:
    //             //     return triegraph_from_graph_impl<TrieGraphBuilderBT>(
    //             //             std::move(graph), s, NoSkip {});
    //             // case Algo::POINT_BFS:
    //             //     return triegraph_from_graph_impl<TrieGraphBuilderPBFS>(
    //             //             std::move(graph), s, NoSkip {});
    //             // case Algo::NODE_BFS:
    //             //     return triegraph_from_graph_impl<TrieGraphBuilderNBFS>(
    //             //             std::move(graph), s, NoSkip {});
    //             default:
    //                 throw 0;
    //         }
    //     } else {
    //         switch (s.algo) {
    //             // case Algo::BACK_TRACK:
    //             //     return triegraph_from_graph_impl<TrieGraphBuilderBT>(
    //             //             std::move(graph), s, SkipEvery { s.skip_every });
    //             // case Algo::POINT_BFS:
    //             //     return triegraph_from_graph_impl<TrieGraphBuilderPBFS>(
    //             //             std::move(graph), s, SkipEvery { s.skip_every });
    //             default:
    //                 throw 0;
    //         }
    //     }
    // }

    // using vec_pairs = std::vector<std::pair<Kmer, typename LetterLocData::LetterLoc>>;

    // template<typename Builder>
    // static vec_pairs pairs_from_graph(const Graph &graph, Settings s, Settings::NoSkip) {
    //     init(s);
    //     // std::cerr << "settings:\n" << s;
    //     auto lloc = LetterLocData(graph);
    //     // std::cerr << "total locations " << lloc.num_locations << std::endl;
    //     auto scope = Logger::get().begin_scoped("builder wrap");
    //     return Builder(graph, lloc).get_pairs(lloc, s.cut_early_threshold);
    // }

    // template <typename Builder>
    // static vec_pairs pairs_from_graph(const Graph &graph,
    //         Settings s, Settings::SkipEvery skip_every) {
    //     init(s);
    //     // std::cerr << "settings:\n" << s;
    //     auto lloc = LetterLocData(graph);
    //     auto sp = ConnectedComponents(graph).compute_starting_points();
    //     auto ss = SparseStarts(graph);
    //     auto lsp = ss.compute_starts_every(skip_every.n, sp);
    //     { auto _ = std::move(sp); }
    //     auto scope = Logger::get().begin_scoped("builder wrap");
    //     return Builder(graph, lloc).get_pairs(lsp, s.cut_early_threshold);
    // }

    // template<typename Builder>
    // static TrieGraph triegraph_from_graph_impl(Graph &&graph, Settings s, Settings::NoSkip) {
    //     auto lloc = LetterLocData(graph);
    //     auto pairs = Builder(graph, lloc).get_pairs(lloc, s.cut_early_threshold);
    //     auto td = TrieData(std::move(pairs), lloc);
    //     return TrieGraph(TrieGraphData(
    //                 std::move(graph), std::move(lloc), std::move(td)));
    // }

    // template <typename Builder>
    // static TrieGraph triegraph_from_graph_impl(Graph &&graph,
    //         Settings s, Settings::SkipEvery skip_every) {
    //     auto lloc = LetterLocData(graph);
    //     auto sp = ConnectedComponents(graph).compute_starting_points();
    //     auto ss = SparseStarts(graph);
    //     auto lsp = ss.compute_starts_every(skip_every.n, sp);
    //     { auto _ = std::move(sp); }
    //     auto pairs = Builder(graph, lloc).get_pairs(lsp, s.cut_early_threshold);
    //     { auto _ = std::move(ss); }
    //     auto td = TrieData(std::move(pairs), lloc);
    //     return TrieGraph(TrieGraphData(
    //                 std::move(graph), std::move(lloc), std::move(td)));
    // }

    // struct FixedKKmer {}; struct DynamicKKmer {};
    // using KmerTag = std::conditional_t<kmer_fixed_k, FixedKKmer, DynamicKKmer>;

    // intended for test use
    static void kmer_set_depth(u16 trie_depth) {
        Kmer::set_settings(KmerSettings::from_depth<typename Cfg::KmerHolder>(trie_depth));
    }

    // static void init(Settings s = {}) {
    //     init(s, KmerTag {});
    // }

    // static void init(Settings s, FixedKKmer) {
    //     if (s.trie_depth != Kmer::K) {
    //         throw "trie depth doesn't match Kmer::K";
    //     }
    // }

    // static void init(Settings s, DynamicKKmer) {
    //     using Holder = Kmer::Holder;
    //     auto on_mask = Holder(1) << (sizeof(Holder) * BITS_PER_BYTE - 1);
    //     Logger::get().log("setting",
    //             "trie_depth =", s.trie_depth,
    //             "on_mask =", std::hex, on_mask, std::dec);
    //     Kmer::setK(s.trie_depth, on_mask);
    // }

    // static void prep_pairs(auto &pairs) {
    //     {
    //         auto st = Logger::get().begin_scoped("sorting pairs");
    //         std::ranges::sort(pairs);
    //     }
    //     {
    //         auto st = Logger::get().begin_scoped("removing dupes");
    //         auto sr = std::ranges::unique(pairs);
    //         auto new_size = sr.begin() - pairs.begin();
    //         pairs.resize(new_size);
    //     }
    // }

};

} /* namespace triegraph */

#endif /* __MANAGER_H__ */
