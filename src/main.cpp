#include "dna_config.h"
#include "manager.h"
#include "util/logger.h"

#include <assert.h>
#include <string>

using namespace std::literals;

using TG = triegraph::Manager<triegraph::dna::DnaConfig<0>>;
using Logger = triegraph::Logger;

typename TG::vec_pairs get_pairs(const TG::Graph &graph, TG::Settings s, decltype(TG::Settings::algo) algo) {
    s.algo = algo;
    if (algo == TG::Settings::BFS) {
        return TG::pairs_from_graph<TG::TrieGraphBuilderBFS>(
                graph, s, TG::Settings::NoSkip {});
    } else if (algo == TG::Settings::BACK_TRACK) {
        return TG::pairs_from_graph<TG::TrieGraphBuilderBT>(
                graph, s, TG::Settings::NoSkip {});
    } else if (algo == TG::Settings::POINT_BFS) {
        return TG::pairs_from_graph<TG::TrieGraphBuilderPBFS>(
                graph, s, TG::Settings::NoSkip {});
    }
    assert(0);
    return {};
}

typename TG::TrieData get_td(const TG::Graph &graph, const TG::LetterLocData &lloc,
        TG::Settings s, decltype(TG::Settings::algo) algo) {
    auto pairs = get_pairs(graph, s, algo);
    auto res = TG::TrieData(pairs, lloc);

    {
        auto scope = Logger::get().begin_scoped("sanity check");
        res.sanity_check(pairs, lloc);
    }

    std::cerr << "T2G Histogrm:" << std::endl;
    res.t2g_histogram().print(std::cerr);
    std::cerr << "G2T Histogrm:" << std::endl;
    res.g2t_histogram().print(std::cerr);
    auto nkmers = std::ranges::distance(res.trie2graph.keys());
    auto nlocs = std::ranges::distance(res.graph2trie.keys());
    std::cerr << "num kmers: " << nkmers << std::endl;
    std::cerr << "all kmers: " << TG::TrieData::total_kmers() << std::endl;
    std::cerr << "num locs: " << nlocs << std::endl;
    std::cerr << "all locs: " << lloc.num_locations << std::endl;
    std::cerr << "ff: " << double(nkmers) / nlocs << std::endl;
    std::cerr << "used kmers: " << double(nkmers) / TG::TrieData::total_kmers() << std::endl;

    return res;
}

void print_pairs(const TG::Graph &graph, const TG::LetterLocData &lloc,
        TG::vec_pairs pairs) {

    {
        auto st = Logger::get().begin_scoped("sorting pairs");
        std::ranges::sort(pairs);
    }
    {
        auto st = Logger::get().begin_scoped("removing dupes");
        auto sr = std::ranges::unique(pairs);
        auto new_size = sr.begin() - pairs.begin();
        pairs.resize(new_size);
    }
    auto st = Logger::get().begin_scoped("printing");
    for (const auto &p : pairs) {
        TG::NodePos np = lloc.expand(p.second);
        const auto &node = graph.node(np.node);
        std::cout << p << " " << np.node << "(" << node.seg_id << "):"
            << np.pos << "/" << node.seg.size() << std::endl;
    }
}

int main(int argc, char *argv[]) {
    assert(argc >= 2);

    try {
        if (argv[1] == "pairs"s || argv[1] == "print-pairs"s || argv[1] == "td"s) {
            std::string gfa_file = argv[2];
            std::string algo = argv[3];

            assert(algo == "bfs" || algo == "back_track" || algo == "pbfs");

            auto algo_v = algo == "bfs" ?
                TG::Settings::BFS : algo == "back_track" ?
                TG::Settings::BACK_TRACK : TG::Settings::POINT_BFS;
            auto graph = TG::Graph::from_file(gfa_file, {});
            auto lloc = TG::LetterLocData(graph);
            TG::Settings s {
                .trie_depth = triegraph::log4_ceil(lloc.num_locations)
            };
            TG::init(s);

            if (argv[1] == "pairs"s || argv[1] == "print-pairs"s) {
                auto pairs = get_pairs(graph, s, algo_v);

                if (argv[1] == "print-pairs"s)
                    print_pairs(graph, lloc, std::move(pairs));
            } else if (argv[1] == "td"s) {
                auto td = get_td(graph, lloc, s, algo_v);
            }
        } else {
            for (int i = 0; i < argc; ++i) {
                std::cerr << i << " '" <<  argv[i] << "'" << std::endl;
            }
        }
    } catch (const char *e) {
        std::cerr << "got exception " << e << std::endl;
    }

    return 0;
}
