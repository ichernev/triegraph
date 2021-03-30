#include "dna_config.h"
#include "manager.h"

#include <assert.h>
#include <string>

using namespace std::literals;

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

auto time_diff_ms(auto a, auto b) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
            b - a).count();
}

typename TG::TrieData get_td(const TG::Graph &graph, const TG::LetterLocData &lloc,
        TG::Settings s, decltype(TG::Settings::algo) algo) {
    auto time_01 = std::chrono::steady_clock::now();
    auto pairs = get_pairs(graph, s, algo);
    auto time_02 = std::chrono::steady_clock::now();
    std::cerr << "pairs: " << time_diff_ms(time_01, time_02) << "ms" << std::endl;
    auto res = TG::TrieData(pairs, lloc);
    auto time_03 = std::chrono::steady_clock::now();
    std::cerr << "TD: " << time_diff_ms(time_02, time_03) << "ms" << std::endl;
    res.sanity_check(pairs, lloc);

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

// typename TG::TrieData get_fake_td(const TG::Graph &graph, const TG::LetterLocData &lloc,
//         TG::Settings s, decltype(TG::Settings::algo) algo) {
//     std::cerr << "YEEES" << std::endl;
//     // auto time_01 = std::chrono::steady_clock::now();
//     // auto pairs = get_pairs(graph, s, algo);
//     auto time_02 = std::chrono::steady_clock::now();
//     // std::cerr << "pairs: " << time_diff_ms(time_01, time_02) << "ms" << std::endl;
//     auto pairs = std::vector<std::pair<TG::Kmer, TG::LetterLoc>> {};
//     { auto _ = std::move(pairs); }
//     std::cerr << "after move" << std::endl;
//     // auto res = TG::TrieData(pairs, lloc);
//     auto time_03 = std::chrono::steady_clock::now();
//     std::cerr << "TD: " << time_diff_ms(time_02, time_03) << "ms" << std::endl;
// }

int main(int argc, char *argv[]) {
    assert(argc >= 2);

    if (argv[1] == "pairs"s) {
        std::string gfa_file = argv[2];
        std::string algo = argv[3];

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
        auto pairs = get_pairs(graph, s, algo_v);

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
    } else if (argv[1] == "td"s) {
        std::string gfa_file = argv[2];
        std::string algo = argv[3];

        assert(algo == "bfs" || algo == "back_track" || algo == "pbfs");

        auto algo_v = algo == "bfs" ?
            TG::Settings::BFS : algo == "back_track" ?
            TG::Settings::BACK_TRACK : TG::Settings::POINT_BFS;
        auto graph = TG::Graph::from_file(gfa_file, { .add_reverse_complement = true });
        auto lloc = TG::LetterLocData(graph);
        TG::Settings s;
        s.add_reverse_complement = true;
        s.trie_depth = triegraph::log4_ceil(lloc.num_locations) + 1;
        TG::init(s);

        std::cerr << "Hello nurse" << std::endl;
        std::cerr << "calling get_td" << std::endl;
        auto td = get_td(graph, lloc, s, algo_v);
    } else {
        for (int i = 0; i < argc; ++i) {
            std::cerr << i << " '" <<  argv[i] << "'" << std::endl;
        }
    }

    return 0;
}
