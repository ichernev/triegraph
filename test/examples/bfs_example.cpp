#include "triegraph/dna_config.h"
#include "triegraph/manager.h"
#include "triegraph/util/logger.h"
#include "triegraph/util/cmdline.h"
#include "triegraph/util/human.h"

#include <assert.h>
#include <string>
#include <limits>
#include <set>

template <typename TG>
typename TG::TrieData get_td(
        const typename TG::Graph &graph,
        const typename TG::LetterLocData &lloc,
        auto &&cfg) {
    auto ks = triegraph::KmerSettings::from_seed_config<typename TG::KmerHolder>(
            lloc.num_locations, cfg);
    // Use NBFS
    auto pairs = TG::template graph_to_pairs<typename TG::TrieBuilderNBFS>(
            graph, lloc, std::move(ks),
            TG::TrieBuilderNBFS::Settings::from_config(cfg),
            lloc);
    pairs.sort_by_fwd().unique();
    // TG::prep_pairs(pairs);
    auto res = TG::pairs_to_triedata(std::move(pairs), lloc);

    auto stats = res.stats();
    std::cerr << stats << std::endl;
    std::cerr << "all kmers: " << TG::TrieData::total_kmers()
        << " used: " << double(stats.num_kmers) / TG::TrieData::total_kmers() << std::endl;
    std::cerr << "all locs: " << lloc.num_locations
        << " used: " << double(stats.num_locs) / lloc.num_locations << std::endl;

    return res;
}

template <typename TG>
void bfs(const typename TG::TrieGraph &tg) {
    // location + previous index
    std::vector<std::pair<typename TG::Handle, size_t>> queue;
    std::unordered_set<typename TG::Handle> visited;

    auto no_prev = std::numeric_limits<size_t>::max();
    queue.emplace_back(tg.root_handle(), no_prev);
    visited.insert(tg.root_handle());
    for (size_t i = 0; i < queue.size(); ++i) {
        const auto& [h, prev] = queue[i];
        std::cerr << "Traversing " << i << ": " << h << " from " << prev << std::endl;
        for (const auto& ee : tg.next_edit_edges(h)) {
            if (ee.edit == TG::EditEdge::MATCH && !visited.contains(ee.handle)) {
                queue.emplace_back(ee.handle, i);
                visited.insert(ee.handle);
            }
        }
    }
}

int main(int argc, char* argv[]) {
    using TG = triegraph::Manager<triegraph::dna::DnaConfig<0>>;

    auto cmdline = triegraph::CmdLine(argc, (const char**) argv);
    auto graph_file = cmdline.positional[0];

    // read the graph
    auto graph = TG::Graph::from_file(graph_file, {});
    std::cerr << "----------- GRAPH ------------" << std::endl;
    std::cerr << graph;
    std::cerr << "----------- END GRAPH ------------" << std::endl;
    // enumerate all graph locations
    auto lloc = TG::LetterLocData(graph);
    // compute the trie (TrieData)
    auto td = get_td<TG>(graph, lloc, cmdline);
    // create traversable TrieGraph from Graph, LLoc and TrieData
    auto tg = TG::TrieGraph(TG::TrieGraphData(std::move(graph), std::move(lloc), std::move(td)));

    bfs<TG>(tg);

    return 0;
}
