#include "rgfa_graph.h"
#include "dna_letter.h"
#include "str.h"

#include <algorithm>
#include <assert.h>

using namespace triegraph;

using DnaStr = Str<dna::DnaLetter, u32>;

static void test_small() {
    auto graph = RgfaGraph<DnaStr, u32>::from_file("data/simpler.gfa");

    assert(graph.nodes.size() == 7);
    assert(graph.edges.size() == 8 * 2);
    assert(graph.edge_start.size() == graph.nodes.size());
    assert(graph.redge_start.size() == graph.nodes.size());

    // std::cerr << graph;

    auto edge_helper = graph.forward_from(0);
    assert(std::distance(edge_helper.begin(), edge_helper.end()) == 2);

    edge_helper = graph.forward_from(1);
    assert(std::distance(edge_helper.begin(), edge_helper.end()) == 2);

    edge_helper = graph.forward_from(2);
    assert(std::distance(edge_helper.begin(), edge_helper.end()) == 1);

    edge_helper = graph.forward_from(3);
    assert(std::distance(edge_helper.begin(), edge_helper.end()) == 0);
}

template <typename ITH> /* iter helper */
std::vector<u32> node_collector(ITH ith) {
    std::vector<u32> res;
    std::transform(
            ith.begin(), ith.end(),
            std::back_inserter(res),
            [](auto e) { return e.node_id; });
    std::sort(res.begin(), res.end());
    return res;
}

static void test_revcomp() {
    auto graph = RgfaGraph<DnaStr, u32>::Builder()
        .add_node(DnaStr("ac"), "s1")
        .add_node(DnaStr("gg"), "s2")
        .add_node(DnaStr("acg"), "s3")
        .add_node(DnaStr("ac"), "s4")
        .add_edge("s1", "s2")
        .add_edge("s1", "s3")
        .add_edge("s2", "s4")
        .add_edge("s3", "s4")
        .build();
    graph.add_reverse_complement();

    // std::cerr << graph;

    assert(graph.nodes[4].seg.to_str() == "gt");
    assert(graph.nodes[5].seg.to_str() == "cc");
    assert(graph.nodes[6].seg.to_str() == "cgt");
    assert(graph.nodes[7].seg.to_str() == "gt");

    assert((node_collector(graph.forward_from(7)) == std::vector<u32>{5, 6}));
    assert((node_collector(graph.forward_from(6)) == std::vector<u32>{4}));
    assert((node_collector(graph.forward_from(5)) == std::vector<u32>{4}));
    assert((node_collector(graph.forward_from(4)) == std::vector<u32>{}));
}

int main() {
    test_small();
    test_revcomp();

    return 0;
}
