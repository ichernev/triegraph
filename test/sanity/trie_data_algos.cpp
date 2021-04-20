#include "dna_config.h"
#include "manager.h"

#include "testlib/test.h"

using namespace triegraph;
using TG = Manager<dna::DnaConfig<0>>;

std::size_t hash_pairs(const TG::vec_pairs &pairs) {
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
    // TG::prep_pairs(pairs);
    return hash_pairs(pairs);
}

// static void test_algo_pairs_eq(std::string gfa_file) {
//     auto graph = TG::Graph::from_file(gfa_file, {});
//     auto lloc = TG::LetterLocData(graph);
//     auto s = TG::Settings {
//         .trie_depth = log4_ceil(lloc.num_locations),
//         // .trie_depth = (td_abs == 0 ?
//         //     triegraph::log4_ceil(lloc.num_locations) + td_rel :
//         //     td_abs)
//     };
//     std::vector<std::size_t> hashes;
//     for (auto algo : TG::algorithms) {
//         auto pairs = get_pairs(graph, s, algo);
//         TG::prep_pairs(pairs);
//         // std::cerr << "np " << pairs.size() << std::endl;
//         hashes.emplace_back(hash_pairs(pairs));
//     }
//     if (hashes.size() > 1) {
//         if (std::ranges::find_if(hashes, [&](const auto &h) { return h != hashes[0]; }) != hashes.end()) {
//             std::cerr << "hashes differ" << std::endl;
//             for (size_t i = 0; i < TG::algorithms.size(); ++i) {
//                 std::cerr << TG::algorithms[i] << " " << hashes[i] << std::endl;
//             }
//             assert(0);
//         }
//     }
// }

template <const char *filename, std::size_t expected_hash>
struct TestAlgos {
    void define_tests() {
        for (auto algo : TG::algorithms) {
            test::define_test(TG::algo_name(algo), [algo] {
                test_algo(filename, algo);
                // assert(test_algo(filename, algo) == expected_hash);
            });
        }
    }
};


int m = test::define_module(__FILE__, [] {
    static const char pasgal[] = "data/pasgal-MHC1.gfa";
    static const char hg22[] = "data/hg_22_nn.gfa";
    static const char hg22_linear[] = "data/HG_22_linear.gfa";
    test::register_test_class<TestAlgos<pasgal, 2158758768>>("pasgal");
    test::register_test_class<TestAlgos<hg22, 2232310820>>("hg22");
    test::register_test_class<TestAlgos<hg22_linear, 21429287>>("hg22 linear");

// test::define_test("pasgal", [] {
//     test_algo_pairs_eq("data/pasgal-MHC1.gfa");
// });
// test::define_test("hg22", [] {
//     test_algo_pairs_eq("data/hg_22_nn.gfa");
// });
// test::define_test("hg22 linear", [] {
//     test_algo_pairs_eq("data/HG_22_linear.gfa");
// });
});
