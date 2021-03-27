#include "dna_config.h"
#include "manager.h"

#include <assert.h>

using TG = triegraph::Manager<triegraph::dna::DnaConfig<0>>;

typename TG::vec_pairs get_pairs(const TG::Graph &graph, TG::Settings s, decltype(TG::Settings::algo) algo) {
    s.algo = algo;
    if (algo == TG::Settings::BFS) {
        return TG::pairs_from_graph<TG::TrieGraphBuilderBFS>(graph, s, TG::Settings::NoSkip {});
    } else if (algo == TG::Settings::BACK_TRACK) {
        return TG::pairs_from_graph<TG::TrieGraphBuilderBT>(graph, s, TG::Settings::NoSkip {});
    } else if (algo == TG::Settings::POINT_BFS) {
        return TG::pairs_from_graph<TG::TrieGraphBuilderPBFS>(graph, s, TG::Settings::NoSkip {});
    }
    assert(0);
    return {};
}

int main(int argc, char *argv[]) {
    assert(argc >= 2);
    std::string gfa_file = argv[1];
    std::string algo = argv[2];

    assert(algo == "bfs" || algo == "back_track" || algo == "pbfs");

    auto algo_v = algo == "bfs" ?
        TG::Settings::BFS : algo == "back_track" ?
        TG::Settings::BACK_TRACK : TG::Settings::POINT_BFS;
    auto graph = TG::Graph::from_file(gfa_file, { .add_reverse_complement = true });
    auto lloc = TG::LetterLocData(graph);
    TG::Settings s;
    s.add_reverse_complement = true;
    s.trie_depth = triegraph::log4_ceil(lloc.num_locations);
    TG::init(s);
    auto pairs = get_pairs(graph, {}, algo_v);

    std::ranges::sort(pairs);
    auto sr = std::ranges::unique(pairs);
    auto new_size = sr.begin() - pairs.begin();
    std::cerr << "removed " << pairs.size() - new_size << " duplicate pairs" << std::endl;
    pairs.resize(new_size);
    for (const auto &p : pairs) {
        TG::NodePos np = lloc.expand(p.second);
        const auto &node = graph.node(np.node);
        std::cout << p << " " << np.node << "(" << node.seg_id << "):" << np.pos << "/" << node.seg.size() << std::endl;
    }

    return 0;
}
