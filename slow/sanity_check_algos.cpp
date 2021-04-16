#include "dna_config.h"
#include "manager.h"

#include "helper.h"

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

static void test_algo_pairs_eq(std::string gfa_file) {
    auto graph = TG::Graph::from_file(gfa_file, {});
    auto lloc = TG::LetterLocData(graph);
    auto s = TG::Settings {
        .trie_depth = log4_ceil(lloc.num_locations),
        // .trie_depth = (td_abs == 0 ?
        //     triegraph::log4_ceil(lloc.num_locations) + td_rel :
        //     td_abs)
    };
    std::vector<std::size_t> hashes;
    for (auto algo : TG::algorithms) {
        auto pairs = get_pairs(graph, s, algo);
        TG::prep_pairs(pairs);
        // std::cerr << "np " << pairs.size() << std::endl;
        hashes.emplace_back(hash_pairs(pairs));
    }
    if (hashes.size() > 1) {
        if (std::ranges::find_if(hashes, [&](const auto &h) { return h != hashes[0]; }) != hashes.end()) {
            std::cerr << "hashes differ" << std::endl;
            for (size_t i = 0; i < TG::algorithms.size(); ++i) {
                std::cerr << TG::algorithms[i] << " " << hashes[i] << std::endl;
            }
            assert(0);
        }
    }
}

int m = test::define_module(__FILE__, [] {
test::define_test("pasgal pairs hash", [] {
    test_algo_pairs_eq("data/pasgal-MHC1.gfa");
});
test::define_test("hg 22 non-linear", [] {
    test_algo_pairs_eq("data/hg_22_nn.gfa");
});
test::define_test("hg 22 linear", [] {
    test_algo_pairs_eq("data/HG_22_linear.gfa");
});
});
