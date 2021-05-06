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

    static void value_iter_check(u64 max, const auto &mmap, auto &&view) {
        auto beg = view.begin(); auto end = view.end();
        // std::cerr << "max is " << max << std::endl;
        for (u64 i = 0; i < max; ++i) {
            // if (i % 100000 == 0) {
            //     // std::cerr << "now at i " << i << std::endl;
            // }
            if (beg != end && (*beg).first == i) {
                assert(mmap.contains(i));
                auto vv0 = test::sorted(mmap.values_for(i));
                auto vv = iter_pair(vv0.begin(), vv0.end());
                assert(!vv.empty());
                while (!vv.empty()) {
                    assert(beg != end);
                    assert(*vv == (*beg).second);
                    ++vv;
                    ++beg;
                }
            } else {
                // test empty range
                assert(!mmap.contains(i));
                assert(mmap.values_for(i).empty());
                auto er = mmap.equal_range(i);
                assert(er.first == er.second);
            }
        }
    }

    static void sanity_check(
            const TrieData &td,
            typename TrieData::VectorPairs &pairs,
            // std::vector<std::pair<typename TrieData::Kmer, typename TrieData::LetterLoc>> pairs,
            const typename TrieData::LetterLocData &letter_loc) {
        // using LetterLoc = TrieData::LetterLoc;
        // using Kmer = TrieData::Kmer;

        auto &trie2graph = td.trie2graph;
        auto &graph2trie = td.graph2trie;
        u64 maxkmer = td.total_kmers();
        // test trie2graph

        pairs.sort_by_fwd();
        assert(test::equal_sorted(trie2graph, pairs.fwd_pairs()));
        value_iter_check(maxkmer, trie2graph, pairs.fwd_pairs());
        pairs.sort_by_rev();
        assert(test::equal_sorted(graph2trie, pairs.rev_pairs()));
        value_iter_check(letter_loc.num_locations, graph2trie, pairs.rev_pairs());
    }

    static void test_with_file(std::string gfa_file) {
        auto graph = TG::Graph::from_file(gfa_file, {});
        auto lloc = TG::LetterLocData(graph);
        auto pairs = TG::graph_to_pairs<TG::TrieBuilderNBFS>(
                graph,
                lloc,
                KmerSettings::from_seed_config(lloc.num_locations, MapCfg {}),
                {},
                lloc);
        auto pairs_copy = TG::VectorPairs {};
        std::ranges::copy(pairs.fwd_pairs(), std::back_inserter(pairs_copy));
        auto td = TrieData(std::move(pairs), lloc);
        // Do sort+uniq after TrieData creation to make sure TD works with
        // non-sorted, non-unique pairs
        pairs_copy.sort_by_rev().unique();
        sanity_check(td, pairs_copy, lloc);
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
    TG::VectorPairs,
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
    TG::VectorPairs,
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
    TG::VectorPairs,
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

using TrieDataDMM_SV0 = TrieData<
    TG::Kmer,
    TG::LetterLocData,
    TG::VectorPairs,
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
        SortedVector<u32>>,
    true>;

int m = test::define_module(__FILE__, [] {
    TrieDataTester<TG::TrieData>::define_tests("Default");
    TrieDataTester<TrieDataSMM>::define_tests("SMM");
    TrieDataTester<TrieDataHMM>::define_tests("HMM");
    TrieDataTester<TrieDataDMM_SV>::define_tests("DMM_SV");
    TrieDataTester<TrieDataDMM_SV0>::define_tests("DMM_SV0");
});
