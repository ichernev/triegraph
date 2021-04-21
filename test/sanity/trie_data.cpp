#include "testlib/test.h"

#include "dna_config.h"
#include "manager.h"

#include "util/hybrid_multimap.h"
#include "util/simple_multimap.h"

using namespace triegraph;
using TG = Manager<dna::DnaConfig<0>>;

template <typename TrieData>
struct TrieDataTester : public test::TestCaseBase {
    std::string graph_file;

    TrieDataTester(std::string &&name, std::string &&graph_file)
        : TestCaseBase(std::move(name)),
          graph_file(std::move(graph_file))
    {}

    static void sanity_check(
            const TrieData &td,
            std::vector<std::pair<typename TrieData::Kmer, typename TrieData::LetterLoc>> pairs,
            const typename TrieData::LetterLocData &letter_loc) {
        using LetterLoc = TrieData::LetterLoc;
        using Kmer = TrieData::Kmer;

        auto &trie2graph = td.trie2graph;
        auto &graph2trie = td.graph2trie;
        // using Fwd = PairFwd<Kmer, LetterLoc>;
        // using Rev = PairRev<Kmer, LetterLoc>;
        u64 maxkmer = td.total_kmers();
        // test trie2graph
        std::ranges::sort(pairs);
        using t2g_vt = decltype(trie2graph.begin())::value_type;
        assert(std::ranges::equal(
                    pairs | std::ranges::views::transform(
                        [](const auto &p) {
                        return t2g_vt(
                                TrieData::KmerCodec::to_int(p.first),
                                p.second);
                        }),
                    test::sorted(trie2graph)));

        auto pbeg = pairs.begin(), pend = pairs.end();
        for (u64 i = 0; i < maxkmer; ++i) {
            if (pbeg != pend && pbeg->first == TrieData::KmerCodec::to_ext(i)) {
                assert(trie2graph.contains(i));
                auto vv0 = test::sorted(trie2graph.values_for(i));
                auto vv = iter_pair(vv0.begin(), vv0.end());
                assert(!vv.empty());
                while (!vv.empty()) {
                    assert(*vv == pbeg->second);
                    ++vv;
                    ++pbeg;
                }
            } else {
                // test empty range
                assert(!trie2graph.contains(i));
                assert(trie2graph.values_for(i).empty());
                auto er = trie2graph.equal_range(i);
                assert(er.first == er.second);
            }
        }

        std::ranges::sort(pairs, PairSwitchComp<Kmer, LetterLoc> {});
        using g2t_vt = decltype(graph2trie.begin())::value_type;
        assert(std::ranges::equal(
                    pairs | std::ranges::views::transform(
                        [](const auto &p) {
                        return g2t_vt(
                                p.second,
                                TrieData::KmerCodec::to_int(p.first));
                        }
                        ),
                    test::sorted(graph2trie)));
        pbeg = pairs.begin(), pend = pairs.end();
        for (u64 i = 0; i < letter_loc.num_locations; ++i) {
            if (pbeg != pend && pbeg->second == i) {
                assert(graph2trie.contains(i));
                auto vv0 = test::sorted(graph2trie.values_for(i));
                auto vv = iter_pair(vv0.begin(), vv0.end());
                assert(!vv.empty());
                while (!vv.empty()) {
                    assert(*vv == TrieData::KmerCodec::to_int(pbeg->first));
                    ++vv;
                    ++pbeg;
                }
            } else {
                // test empty range
                assert(!graph2trie.contains(i));
                assert(graph2trie.values_for(i).empty());
                auto er = graph2trie.equal_range(i);
                assert(er.first == er.second);
            }
        }
    }

    static void test_with_file(std::string gfa_file) {
        auto graph = TG::Graph::from_file(gfa_file, {});
        auto lloc = TG::LetterLocData(graph);
        auto s = TG::Settings {
            .trie_depth = log4_ceil(lloc.num_locations),
            // .trie_depth = (td_abs == 0 ?
            //     triegraph::log4_ceil(lloc.num_locations) + td_rel :
            //     td_abs)
        };
        auto pairs = TG::pairs_from_graph<TG::TrieGraphBuilderBFS>(
                graph, s, TG::Settings::NoSkip {});
        TG::prep_pairs(pairs);
        auto res = TrieData(pairs, lloc);
        sanity_check(res, pairs, lloc);
    }

    virtual void run() {
        test_with_file(graph_file);
    }

    using Self = TrieDataTester;
    static void define_tests(std::string pref) {
        pref += "::";
        test::add_test<Self>(pref + "pasgal", "data/pasgal-MHC1.gfa");
        test::add_test<Self>(pref + "hg22", "data/hg_22_nn.gfa");
        test::add_test<Self>(pref + "hg22_lin", "data/HG_22_linear.gfa");
    }
};

using TrieDataSMM = TrieData<
    TG::Kmer,
    TG::LetterLocData,
    TG::triedata_allow_inner,
    SimpleMultimap<
        typename TG::KmerHolder,
        typename TG::LetterLoc>,
    SimpleMultimap<
        typename TG::LetterLoc,
        typename TG::KmerHolder>>;

using HMMMap = std::unordered_map<u32, std::pair<u32, u32>>;
using TrieDataHMM = TrieData<
    TG::Kmer,
    TG::LetterLocData,
    TG::triedata_allow_inner,
    HybridMultimap<
        typename TG::KmerHolder,
        typename TG::LetterLoc,
        typename TG::LetterLoc,
        HMMMap>,
    HybridMultimap<
        typename TG::LetterLoc,
        typename TG::LetterLoc,
        typename TG::KmerHolder,
        HMMMap>>;

using TrieDataDMM_SV = TrieData<
    TG::Kmer,
    TG::LetterLocData,
    TG::triedata_allow_inner,
    DenseMultimap<
        typename TG::KmerHolder,
        typename TG::LetterLoc,
        typename TG::LetterLoc,
        SortedVector<u32>>,
    DenseMultimap<
        typename TG::LetterLoc,
        typename TG::LetterLoc,
        typename TG::KmerHolder,
        SortedVector<u32>>>;

int m = test::define_module(__FILE__, [] {
    TrieDataTester<TG::TrieData>::define_tests("Default");
    TrieDataTester<TrieDataSMM>::define_tests("SMM");
    TrieDataTester<TrieDataHMM>::define_tests("HMM");
    TrieDataTester<TrieDataDMM_SV>::define_tests("DMM_SV");
});
