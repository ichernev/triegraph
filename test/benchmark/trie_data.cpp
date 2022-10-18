// SPDX-License-Identifier: MPL-2.0
/*
 * Copyright (c) 2021, Iskren Chernev
 */

#include "testlib/test.h"

#include "triegraph/dna_config.h"
#include "triegraph/manager.h"

#include <sparsepp/spp.h>

using namespace triegraph;
using triegraph::dna::CfgFlags;

template <typename TG, typename TrieData = TG::TrieData>
struct TrieDataTester : public test::TestCaseBase {
    std::string graph_file;

    typename TG::LetterLocData lloc;
    typename TG::VectorPairs pairs;
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
        auto lloc = typename TG::LetterLocData(graph);
        // auto s = TG::Settings {
        //     .trie_depth = log4_ceil(lloc.num_locations) + td_rel,
        //     // .trie_depth = (td_abs == 0 ?
        //     //     triegraph::log4_ceil(lloc.num_locations) + td_rel :
        //     //     td_abs)
        // };
        auto pairs = TG::template graph_to_pairs<typename TG::TrieBuilderNBFS>(
                graph, lloc,
                TG::KmerSettings::from_seed_config(lloc.num_locations, MapCfg {
                    "trie-depth-rel", std::to_string(td_rel)}),
                {}, lloc);
        // TG::prep_pairs(pairs);
        this->pairs = std::move(pairs);
        this->lloc = std::move(lloc);
    }

    virtual void run() {
        typename TG::VectorPairs xpairs;
        std::swap(xpairs, this->pairs);
        auto td = TrieData(std::move(xpairs), this->lloc);
        (void) td;
    }

    static void define_tests(std::string pref) {
        std::vector<std::pair<std::string, std::string>> graphs = {
            { "pasgal", "data/pasgal-MHC1.gfa" },
            // { "hg22", "data/hg_22_nn.gfa" },
            // { "hg22_linear", "data/HG_22_linear.gfa" },
            // { "hg01", "tmp/hg_01_nn.gfa" },
            // { "hgc12", "tmp/hg_c12.gfa" },
            // { "hgc123", "tmp/hg_c123.gfa" },
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

using TG = Manager<dna::DnaConfig<0>>;

struct CfgSMM : public dna::DnaConfig<0> { static constexpr u32 TDMapType = 0u; /* use SMM */ };
using TG_SMM = Manager<CfgSMM>;

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
using HMMSPP = spp::sparse_hash_map<u32, std::pair<u32, u32>>;
using TrieDataHMM_SPP = TrieData<
    TG::Kmer,
    TG::LetterLocData,
    TG::VectorPairs,
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

using TG_SV = Manager<dna::DnaConfig<0, CfgFlags::TD_SORTED_VECTOR | CfgFlags::VP_DUAL_IMPL>>;
using TG_SV_CV = Manager<dna::DnaConfig<0, CfgFlags::TD_SORTED_VECTOR | CfgFlags::VP_DUAL_IMPL | CfgFlags::CV_ELEMS>>;
using TG_SV0 = Manager<dna::DnaConfig<0, CfgFlags::TD_SORTED_VECTOR | CfgFlags::VP_DUAL_IMPL | CfgFlags::TD_ZERO_OVERHEAD>>;

int m = test::define_module(__FILE__, [] {
    // TrieDataTester<TG /*, TG::TrieData */>::define_tests("default");
    // TrieDataTester<TG_SMM /*, TrieDataSMM */>::define_tests("SMM");
    // TrieDataTester<TG, TrieDataHMM>::define_tests("HMM");
    // TrieDataTester<TG, TrieDataHMM_SPP>::define_tests("HMM_SPP");
    TrieDataTester<TG_SV/*, TrieDataDMM_SV*/>::define_tests("DMM_SV");
    TrieDataTester<TG_SV_CV/*, TrieDataDMM_SV*/>::define_tests("DMM_SV_CV");
    TrieDataTester<TG_SV0/*, TrieDataDMM_SV0*/>::define_tests("DMM_SV0");
});
