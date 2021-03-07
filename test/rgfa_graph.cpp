#include "rgfa_graph.h"
#include "dna_str.h"

#include <algorithm>
#include <assert.h>

static void test_small() {
    auto graph = RgfaGraph<DnaStr, u32>::from_file("data/simpler.gfa");

    assert(graph.nodes.size() == 7);
    assert(graph.edges.size() == 8 * 2);
    assert(graph.edge_start.size() == graph.nodes.size());
    assert(graph.redge_start.size() == graph.nodes.size());

    std::cerr << graph;
    // for (u32 i = 0; i < graph.nodes.size(); ++i) {
    //     std::cerr << graph.nodes[i].seg_id << "," << i << " ->";
    //     for (const auto &to_node : graph.forward_from(i)) {
    //         std::cerr << " " << to_node.seg_id << "," << to_node.node_id;
    //     }
    //     std::cerr << std::endl;
    // }

    auto edge_helper = graph.forward_from(0);
    assert(std::distance(edge_helper.begin(), edge_helper.end()) == 2);

    edge_helper = graph.forward_from(1);
    assert(std::distance(edge_helper.begin(), edge_helper.end()) == 2);

    edge_helper = graph.forward_from(2);
    assert(std::distance(edge_helper.begin(), edge_helper.end()) == 1);
}

int main() {
    test_small();

    return 0;
}
