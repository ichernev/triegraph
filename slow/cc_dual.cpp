#include "dna_config.h"
#include "manager.h"

#include "util/cmdline.h"
#include "util/logger.h"

using namespace triegraph;

struct Cfg : dna::DnaConfig<0> {
    static constexpr bool triedata_allow_inner = true;
};

using TG = Manager<Cfg>;

template <typename Builder, std::ranges::input_range R>
static TG::vec_pairs get_pairs_impl(
        const TG::Graph &graph,
        const TG::LetterLocData &lloc,
        R&& cc_seed,
        TG::Settings s) {
    return Builder(graph, lloc).get_pairs(std::forward<R>(cc_seed), s.cut_early_threshold);
}

template <std::ranges::input_range R>
static TG::vec_pairs get_pairs(
        const TG::Graph &graph,
        const TG::LetterLocData &lloc,
        std::string algo,
        R&& cc_seed,
        TG::Settings s) {
    if (algo == "bfs") {
        throw -1;
        // return get_pairs_impl<TG::TrieGraphBuilderBFS>(graph, lloc, cc_seed, s);
    } else if (algo == "back_track") {
        return get_pairs_impl<TG::TrieGraphBuilderBT>(graph, lloc, std::forward<R>(cc_seed), s);
    } else if (algo == "pbfs") {
        return get_pairs_impl<TG::TrieGraphBuilderPBFS>(graph, lloc, std::forward<R>(cc_seed), s);
    } else if (algo == "node_bfs") {
        throw -1;
        // return get_pairs_impl<TG::TrieGraphBuilderNBFS>(graph, lloc, cc_seed, s);
    } else {
        throw -1;
    }
}

int main(int argc, char *argv[]) {
    using std::string_literals::operator""s;

    auto cmdline = CmdLine(argc, argv);
    cmdline.debug();

    auto algo_fast = cmdline.get_or<std::string>("algo-fast", "back_track"s);
    auto algo_slow = cmdline.get_or<std::string>("algo-slow", "pbfs"s);
    auto backedge_init = cmdline.get_or<u32>("cc-backedge-init", 4u);
    auto backedge_max_trav = cmdline.get_or<u32>("cc-backedge-max-trav", 2u);
    auto cc_cutoff = cmdline.get_or<u32>("cc-cutoff", 512u);
    auto pbfs_kmer_cutoff = cmdline.get_or<u32>("pbfs-kmer-cutoff", 256u);
    auto gfa_file = cmdline.get<std::string>("graph");
    auto td_rel = cmdline.get_or<i32>("trie-depth-rel", 0);
    auto td_abs = cmdline.get_or<u32>("trie-depth", 0);

    Logger::get().begin("reading graph");
    auto graph = TG::Graph::from_file(gfa_file, {});
    Logger::get().end().begin("building lloc");
    auto lloc = TG::LetterLocData(graph);
    TG::Settings s {
        .trie_depth = td_abs == 0 ?
            triegraph::log4_ceil(lloc.num_locations) + td_rel :
            td_abs,
        .cut_early_threshold = pbfs_kmer_cutoff,
    };
    Logger::get().end().begin("TG::init");
    TG::init(s);

    Logger::get().end().begin("top_order build");
    auto top_ord = TG::TopOrder::Builder(graph).build();
    Logger::get().end().begin("CE building");
    auto ce = TG::ComplexityEstimator(
            graph,
            top_ord,
            s.trie_depth,
            backedge_init,
            backedge_max_trav)
        .compute();

    auto cc_seed_v = std::views::iota(0u)
        | std::views::take(graph.num_nodes())
        | std::views::filter([&ce, cc_cutoff] (auto n_id) {
                // std::cerr << "-WWX " << n_id << std::endl;
                assert(n_id < ce.get_ends().size());
                return ce.get_ends()[n_id] >= cc_cutoff; });

    // std::cerr << "X_____X" << std::endl;
    // std::ranges::copy(cc_seed_v, std::ostream_iterator<TG::NodeLoc>(std::cerr, "\n"));

    Logger::get().end().begin("CCW building");
    auto ccw = TG::ComplexityComponentWalker::Builder(
            graph,
            s.trie_depth)
        .build(cc_seed_v);

    auto num_cc_starts = std::ranges::distance(ccw.cc_starts(graph, s.trie_depth));
    auto num_non_cc_starts = std::ranges::distance(ccw.non_cc_starts(graph, s.trie_depth));
    auto num_all_starts = lloc.num_locations;

    Logger::get().log(
            "cc_starts", num_cc_starts,
            "as %", (num_cc_starts * 100.0) / num_all_starts,
            "non_cc_starts", num_non_cc_starts,
            "as %", (num_non_cc_starts * 100.0) / num_all_starts);

    Logger::get().end().begin("CCW non_cc_starts");
    auto p1 = get_pairs(graph, lloc, algo_fast, ccw.non_cc_starts(graph, s.trie_depth), s);

    Logger::get().end().begin("CCW cc_starts");
    auto p2 = get_pairs(graph, lloc, algo_slow, ccw.cc_starts(graph, s.trie_depth), s);

    Logger::get().end().begin("merge pairs");
    std::ranges::copy(p2, std::back_inserter(p1));
    { auto _ = std::move(p2); }

    Logger::get().end().begin("build TD");
    auto td = TG::TrieData(std::move(p1), lloc);
    Logger::get().end();

    std::cerr << "T2G Histogrm:" << std::endl;
    td.t2g_histogram().print(std::cerr);
    std::cerr << "G2T Histogrm:" << std::endl;
    td.g2t_histogram().print(std::cerr);
    auto nkmers = std::ranges::distance(td.trie2graph.keys());
    auto nlocs = std::ranges::distance(td.graph2trie.keys());
    std::cerr << "num kmers: " << nkmers << std::endl;
    std::cerr << "all kmers: " << TG::TrieData::total_kmers() << std::endl;
    std::cerr << "num locs: " << nlocs << std::endl;
    std::cerr << "all locs: " << lloc.num_locations << std::endl;
    std::cerr << "ff: " << double(nkmers) / nlocs << std::endl;
    std::cerr << "used kmers: " << double(nkmers) / TG::TrieData::total_kmers() << std::endl;

    std::cerr << "t2g elem size: " << td.trie2graph.size() << std::endl;
    std::cerr << "g2t elem size: " << td.graph2trie.size() << std::endl;

    return 0;
}
