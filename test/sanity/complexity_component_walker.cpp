#include "testlib/dna.h"
#include "testlib/test.h"

#include <random>

using TG = test::Manager_RK;

std::vector<TG::NodeLoc> _pick_random_cc_seeds(const TG::Graph &graph,
        triegraph::u32 trie_depth, TG::NodeLoc num_cc) {
    std::vector<TG::NodeLoc> all_short;
    for (TG::NodeLoc i = 0; i < graph.num_nodes(); ++i) {
        if (graph.node(i).seg.size() < trie_depth) {
            all_short.push_back(i);
        }
    }
    std::ranges::shuffle(all_short, std::minstd_rand {});
    if (num_cc < all_short.size())
        all_short.resize(num_cc);
    return all_short;
}

int m = test::define_module(__FILE__, [] {
test::define_test("pasgal + rand cc", [] {
    auto graph = TG::Graph::from_file("data/pasgal-MHC1.gfa");
    auto lloc = TG::LetterLocData(graph);

    triegraph::u32 trie_depth = triegraph::log4_ceil(graph.num_nodes());
    TG::NodeLoc num_cc = 300;

    auto cc_seeds =  _pick_random_cc_seeds(graph, trie_depth, num_cc);
    // auto generator = std::minstd_rand0 {};
    // auto rnd_ids = std::ranges::views::iota(0)
    //     | std::ranges::views::transform([&generator,n=graph.num_nodes()] (auto x) {
    //             auto res = TG::NodeLoc(generator() % n);
    //             std::cerr << "XXID " << res << std::endl;
    //             return res;})
    //     | std::ranges::views::filter([&graph,trie_depth] (TG::NodeLoc node_id) {
    //             auto is_short = graph.node(node_id).seg.size() < trie_depth;;
    //             std::cerr << "NID " << node_id << " " << is_short << std::endl;
    //             return  is_short; })
    //     | std::ranges::views::take(num_cc);

    // std::cerr << "TD " << trie_depth << std::endl;
    // for (auto node_id : rnd_ids) {
    //     std::cerr << "----> " << node_id << " " << graph.node(node_id).seg.size() << std::endl;
    // }
    // std::cerr << "XXXXTD " << trie_depth << std::endl;
    // for (auto node_id : rnd_ids) {
    //     std::cerr << node_id << " " << graph.node(node_id).seg.size() << std::endl;
    // }

    auto ccw = TG::ComplexityComponentWalker::Builder(graph, trie_depth)
        .build(cc_seeds);

    std::vector<TG::NodePos> all_starts;

    // std::cerr << std::endl;
    // for (auto const &p : ccw.cc_starts(graph, trie_depth)) {
    //     std::cerr << p << " " << graph.node(p.node).seg.size() << std::endl;
    // }

    std::ranges::copy(ccw.cc_starts(graph, trie_depth), std::back_inserter(all_starts));
    std::ranges::sort(all_starts, std::less<TG::NodePos> {});
    assert(std::ranges::unique(all_starts).begin() == all_starts.end());

    std::ranges::copy(ccw.non_cc_starts(graph, trie_depth), std::back_inserter(all_starts));
    std::ranges::sort(all_starts, std::less<TG::NodePos> {});
    assert(std::ranges::unique(all_starts).begin() == all_starts.end());

    // TG::NodeLoc needle = 154443;
    // for (auto const &cc : ccw.ccs) {
    //     if (std::ranges::count(cc.incoming, needle)) {
    //         std::cerr << "in inc" << std::endl;
    //     }
    //     if (std::ranges::count(cc.outgoing, needle)) {
    //         std::cerr << "in out" << std::endl;
    //     }
    //     if (std::ranges::count(cc.internal, needle)) {
    //         std::cerr << "in int" << std::endl;
    //     }
    // }
    // if (std::ranges::count(ccw.external, needle)) {
    //     std::cerr << "xx-in ext" << std::endl;
    // }
    // if (std::ranges::count(ccw.incoming, needle)) {
    //     std::cerr << "xx-in inc" << std::endl;
    // }

    // auto alls_x = triegraph::iter_pair(all_starts.begin(), all_starts.end());
    // auto lloc_x = triegraph::iter_pair(lloc.begin(), lloc.end());

    // TG::NodeLoc counter = 0;
    // while (!alls_x.empty() && !lloc_x.empty()) {
    //     if (*alls_x != *lloc_x) {
    //         std::cerr << "XXX" << counter << " " << lloc.num_locations << " "
    //             << *alls_x << " " << *lloc_x << std::endl;
    //         break;
    //     }
    //     ++ alls_x;
    //     ++ lloc_x;
    //     ++ counter;
    // }

    // assert all locations are present
    assert(all_starts.size() == lloc.num_locations);
});

test::define_test("hg_22 + rand cc", [] {
    auto graph = TG::Graph::from_file("data/hg_22_nn.gfa");
    auto lloc = TG::LetterLocData(graph);

    triegraph::u32 trie_depth = triegraph::log4_ceil(graph.num_nodes());
    TG::NodeLoc num_cc = 300;

    auto cc_seeds =  _pick_random_cc_seeds(graph, trie_depth, num_cc);
    // auto generator = std::minstd_rand0 {};
    // auto rnd_ids = std::ranges::views::iota(0)
    //     | std::ranges::views::transform([&generator,n=graph.num_nodes()] (auto x) {
    //             auto res = TG::NodeLoc(generator() % n);
    //             std::cerr << "XXID " << res << std::endl;
    //             return res;})
    //     | std::ranges::views::filter([&graph,trie_depth] (TG::NodeLoc node_id) {
    //             auto is_short = graph.node(node_id).seg.size() < trie_depth;;
    //             std::cerr << "NID " << node_id << " " << is_short << std::endl;
    //             return  is_short; })
    //     | std::ranges::views::take(num_cc);

    // std::cerr << "TD " << trie_depth << std::endl;
    // for (auto node_id : rnd_ids) {
    //     std::cerr << "----> " << node_id << " " << graph.node(node_id).seg.size() << std::endl;
    // }
    // std::cerr << "XXXXTD " << trie_depth << std::endl;
    // for (auto node_id : rnd_ids) {
    //     std::cerr << node_id << " " << graph.node(node_id).seg.size() << std::endl;
    // }

    auto ccw = TG::ComplexityComponentWalker::Builder(graph, trie_depth)
        .build(cc_seeds);

    std::vector<TG::NodePos> all_starts;

    std::ranges::copy(ccw.cc_starts(graph, trie_depth), std::back_inserter(all_starts));
    std::ranges::sort(all_starts, std::less<TG::NodePos> {});
    assert(std::ranges::unique(all_starts).begin() == all_starts.end());

    std::ranges::copy(ccw.non_cc_starts(graph, trie_depth), std::back_inserter(all_starts));
    std::ranges::sort(all_starts, std::less<TG::NodePos> {});
    assert(std::ranges::unique(all_starts).begin() == all_starts.end());

    // TG::NodeLoc needle = 154443;
    // for (auto const &cc : ccw.ccs) {
    //     if (std::ranges::count(cc.incoming, needle)) {
    //         std::cerr << "in inc" << std::endl;
    //     }
    //     if (std::ranges::count(cc.outgoing, needle)) {
    //         std::cerr << "in out" << std::endl;
    //     }
    //     if (std::ranges::count(cc.internal, needle)) {
    //         std::cerr << "in int" << std::endl;
    //     }
    // }
    // if (std::ranges::count(ccw.external, needle)) {
    //     std::cerr << "xx-in ext" << std::endl;
    // }
    // if (std::ranges::count(ccw.incoming, needle)) {
    //     std::cerr << "xx-in inc" << std::endl;
    // }

    // auto alls_x = triegraph::iter_pair(all_starts.begin(), all_starts.end());
    // auto lloc_x = triegraph::iter_pair(lloc.begin(), lloc.end());

    // TG::NodeLoc counter = 0;
    // while (!alls_x.empty() && !lloc_x.empty()) {
    //     if (*alls_x != *lloc_x) {
    //         std::cerr << "XXX" << counter << " " << lloc.num_locations << " "
    //             << *alls_x << " " << *lloc_x << std::endl;
    //         break;
    //     }
    //     ++ alls_x;
    //     ++ lloc_x;
    //     ++ counter;
    // }

    // assert all locations are present
    assert(all_starts.size() == lloc.num_locations);
});
});
