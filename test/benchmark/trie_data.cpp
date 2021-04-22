#include "testlib/test.h"

#include "dna_config.h"
#include "manager.h"

#include <sparsepp/spp.h>

using namespace triegraph;
using TG = Manager<dna::DnaConfig<0>>;

template <typename TrieData>
struct TrieDataTester : public test::TestCaseBase {
    std::string graph_file;

    TG::LetterLocData lloc;
    TG::vec_pairs pairs;
    int td_rel;
    // TrieData td;
    // TrieData::Stats stats;

    TrieDataTester(const std::string &name, const std::string &graph_file, int td_rel)
        : TestCaseBase(std::move(name)),
          graph_file(std::move(graph_file)),
          td_rel(td_rel)
    {}

    virtual void prepare() {
        auto graph = TG::Graph::from_file(graph_file, {});
        auto lloc = TG::LetterLocData(graph);
        auto s = TG::Settings {
            .trie_depth = log4_ceil(lloc.num_locations) + td_rel,
            // .trie_depth = (td_abs == 0 ?
            //     triegraph::log4_ceil(lloc.num_locations) + td_rel :
            //     td_abs)
        };
        auto pairs = TG::pairs_from_graph<TG::TrieGraphBuilderBFS>(
                graph, s, TG::Settings::NoSkip {});
        // TG::prep_pairs(pairs);
        this->pairs = std::move(pairs);
        this->lloc = std::move(lloc);
    }

    virtual void run() {
        TG::vec_pairs xpairs;
        std::swap(xpairs, this->pairs);
        auto td = TrieData(std::move(xpairs), this->lloc);
        (void) td;
    }

    static void define_tests(std::string pref) {
        std::vector<std::pair<std::string, std::string>> graphs = {
            { "pasgal", "data/pasgal-MHC1.gfa" },
            { "hg22", "data/hg_22_nn.gfa" },
            { "hg22_linear", "data/HG_22_linear.gfa" },
        };
        for (const auto &g : graphs) {
            for (int td_rel = 0; td_rel <= 2; ++td_rel) {
                std::ostringstream os;
                os << pref << "::" << g.first << "::+" << td_rel;
                test::add_test<TrieDataTester>(os.str(), g.second, td_rel);
            }
        }

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
using HMMSPP = spp::sparse_hash_map<u32, std::pair<u32, u32>>;
using TrieDataHMM_SPP = TrieData<
    TG::Kmer,
    TG::LetterLocData,
    TG::triedata_allow_inner,
    HybridMultimap<
        typename TG::KmerHolder,
        typename TG::LetterLoc,
        typename TG::LetterLoc,
        HMMSPP>,
    HybridMultimap<
        typename TG::LetterLoc,
        typename TG::LetterLoc,
        typename TG::KmerHolder,
        HMMSPP>>;

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
    // TrieDataTester<TG::TrieData>::define_tests("default");
    // TrieDataTester<TrieDataSMM>::define_tests("SMM");
    // TrieDataTester<TrieDataHMM>::define_tests("HMM");
    TrieDataTester<TrieDataHMM_SPP>::define_tests("HMM_SPP");
    TrieDataTester<TrieDataDMM_SV>::define_tests("DMM_SV");
});