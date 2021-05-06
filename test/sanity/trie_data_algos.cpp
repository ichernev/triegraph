#include "testlib/dna.h"
#include "testlib/test.h"

using namespace triegraph;
using TG = test::Manager_RK;

struct TrieDataBuilderTester : test::TestCaseBase {
    std::string graph_file;
    TG::Algo algo;
    std::size_t expected_hash;

    TrieDataBuilderTester(
            const std::string &name,
            const std::string &graph_file,
            TG::Algo algo,
            std::size_t expected_hash)
        : TestCaseBase(name),
          graph_file(graph_file),
          algo(algo),
          expected_hash(expected_hash)
    {}

    static std::size_t hash_pairs(const TG::VectorPairs &pairs) {
        std::size_t res = 0;
        for (const auto &p : pairs.fwd_pairs()) {
            res ^= std::hash<TG::Kmer/*::Holder*/>{}(p.first);
            res ^= std::hash<TG::LetterLoc>{}(p.second);
        }
        return res;
    }

    static std::size_t test_algo(std::string gfa_file, TG::Algo algo) {
        auto graph = TG::Graph::from_file(gfa_file, {});
        auto lloc = TG::LetterLocData(graph);

        // force PBFS not to cut early
        auto cfg = MapCfg { "trie-builder-pbfs-cut-early-threshold", "50000000" };
        auto pairs = graph_to_pairs(
                graph, lloc,
                KmerSettings::from_seed_config(lloc.num_locations, cfg),
                cfg, algo);
        pairs.sort_by_fwd().unique();
        return hash_pairs(pairs);
    }

    static TG::VectorPairs graph_to_pairs(
            const TG::Graph &graph,
            const TG::LetterLocData &lloc,
            TG::KmerSettings &&kmer_settings,
            const auto &cfg,
            TG::Algo algo) {
        switch (algo) {
            case TG::Algo::LOCATION_BFS:
                return TG::graph_to_pairs<TG::TrieBuilderLBFS>(
                        graph, lloc, std::move(kmer_settings),
                        TG::TrieBuilderLBFS::Settings::from_config(cfg),
                        lloc);
            case TG::Algo::BACK_TRACK:
                return TG::graph_to_pairs<TG::TrieBuilderBT>(
                        graph, lloc, std::move(kmer_settings),
                        TG::TrieBuilderBT::Settings::from_config(cfg),
                        lloc);
            case TG::Algo::POINT_BFS:
                return TG::graph_to_pairs<TG::TrieBuilderPBFS>(
                        graph, lloc, std::move(kmer_settings),
                        TG::TrieBuilderPBFS::Settings::from_config(cfg),
                        lloc);
            case TG::Algo::NODE_BFS:
                return TG::graph_to_pairs<TG::TrieBuilderNBFS>(
                        graph, lloc, std::move(kmer_settings),
                        TG::TrieBuilderNBFS::Settings::from_config(cfg),
                        lloc);
            default:
                throw "Unknown algorithm";
        }
    }


    virtual void run() {
        // std::cerr << test_algo(graph_file, algo) << std::endl;;
        assert(test_algo(graph_file, algo) == expected_hash);
    }

    static void define_tests(std::string graph_slug, std::string graph_file,
            std::size_t expected_hash) {
        for (auto algo : TG::algorithms) {
            test::add_test<TrieDataBuilderTester>(
                    graph_slug + "::" + TG::algo_name(algo),
                    graph_file, algo, expected_hash);
        }
    }
};

int m = test::define_module(__FILE__, [] {
    TrieDataBuilderTester::define_tests("pasgal", "data/pasgal-MHC1.gfa", 2158758768u);
    TrieDataBuilderTester::define_tests("hg22", "data/hg_22_nn.gfa", 2232310820u);
    TrieDataBuilderTester::define_tests("hg22_linear", "data/HG_22_linear.gfa", 21429287u);
});
