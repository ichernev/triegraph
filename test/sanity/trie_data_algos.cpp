#include "dna_config.h"
#include "manager.h"

#include "testlib/test.h"

using namespace triegraph;
using TG = Manager<dna::DnaConfig<0>>;

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

    static std::size_t hash_pairs(const TG::vec_pairs &pairs) {
        std::size_t res = 0;
        for (const auto &p : pairs) {
            res ^= std::hash<TG::Kmer>{}(p.first);
            res ^= std::hash<TG::LetterLoc>{}(p.second);
        }
        return res;
    }

    static typename TG::vec_pairs get_pairs(
            const TG::Graph &graph, TG::Settings s, TG::Algo algo) {
        s.algo = algo;
        if (algo == TG::Algo::BFS) {
            return TG::pairs_from_graph<TG::TrieGraphBuilderBFS>(
                    graph, s, TG::Settings::NoSkip {});
        } else if (algo == TG::Algo::BACK_TRACK) {
            return TG::pairs_from_graph<TG::TrieGraphBuilderBT>(
                    graph, s, TG::Settings::NoSkip {});
        } else if (algo == TG::Algo::POINT_BFS) {
            return TG::pairs_from_graph<TG::TrieGraphBuilderPBFS>(
                    graph, s, TG::Settings::NoSkip {});
        } else if (algo == TG::Algo::NODE_BFS) {
            return TG::pairs_from_graph<TG::TrieGraphBuilderNBFS>(
                    graph, s, TG::Settings::NoSkip {});
        }
        assert(0);
        return {};
    }

    static std::size_t test_algo(std::string gfa_file, TG::Algo algo) {
        auto graph = TG::Graph::from_file(gfa_file, {});
        auto lloc = TG::LetterLocData(graph);
        auto s = TG::Settings {
            .trie_depth = log4_ceil(lloc.num_locations),
            // .trie_depth = (td_abs == 0 ?
            //     triegraph::log4_ceil(lloc.num_locations) + td_rel :
            //     td_abs)
        };
        std::vector<std::size_t> hashes;
        auto pairs = get_pairs(graph, s, algo);
        TG::prep_pairs(pairs);
        return hash_pairs(pairs);
    }

    virtual void run() {
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
