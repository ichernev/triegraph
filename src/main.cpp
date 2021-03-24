#include "dna_config.h"
#include "manager.h"

#include <iostream>

using TG = triegraph::Manager<triegraph::dna::DnaConfig<13>>;

int main() {
    auto tg = TG::triegraph_from_rgfa_file<TG::TrieGraphBTBuilder>(
            "data/pasgal-MHC1.gfa", TG::Settings());
    auto [a, b, c] = tg.graph_size();
    auto [d, e, f] = tg.trie_size();

    std::cerr << a << " " << b << " " << c << std::endl;
    std::cerr << d << " " << e << " " << f << std::endl;

    for (;;) { }

    return 0;
}
