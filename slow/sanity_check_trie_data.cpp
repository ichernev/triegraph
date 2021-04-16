#include "dna_config.h"
#include "manager.h"

#include "helper.h"

using namespace triegraph;
using TG = Manager<dna::DnaConfig<0>>;

static void sanity_check_trie_data(std::string gfa_file) {
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
    auto res = TG::TrieData(pairs, lloc);
    res.sanity_check(pairs, lloc);
}

int m = test::define_module(__FILE__, [] {
test::define_test("pasgal pairs hash", [] {
    sanity_check_trie_data("data/pasgal-MHC1.gfa");
});
test::define_test("hg 22 non-linear", [] {
    sanity_check_trie_data("data/hg_22_nn.gfa");
});
test::define_test("hg 22 linear", [] {
    sanity_check_trie_data("data/HG_22_linear.gfa");
});
});
