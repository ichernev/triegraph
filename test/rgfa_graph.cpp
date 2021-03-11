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

    std::cerr << graph;

    auto edge_helper = graph.forward_from(0);
    assert(std::distance(edge_helper.begin(), edge_helper.end()) == 2);

    edge_helper = graph.forward_from(1);
    assert(std::distance(edge_helper.begin(), edge_helper.end()) == 2);

    edge_helper = graph.forward_from(2);
    assert(std::distance(edge_helper.begin(), edge_helper.end()) == 1);

    edge_helper = graph.forward_from(3);
    assert(std::distance(edge_helper.begin(), edge_helper.end()) == 0);
}

int main() {
    test_small();

    return 0;
}
