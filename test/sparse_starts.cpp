#include "dna_config.h"
#include "graph/sparse_starts.h"
#include "manager.h"

#include <iostream>
#include <iterator>
#include <assert.h>

using TG = triegraph::Manager<triegraph::dna::DnaConfig<0>>;

static void test_iter_concept() {
    auto func = [](std::forward_iterator auto it) {};
    func(triegraph::SparseStarts<TG::Graph, TG::NodePos>::const_iterator());
}

static void test_small_linear() {
    auto g = TG::Graph::Builder()
        .add_node(TG::Str("acgtacgt"), "s1")
        .build();

    auto ss = triegraph::SparseStarts<TG::Graph, TG::NodePos>(g);
    auto starts = ss.compute_starts_every(2, {0});

    std::ranges::equal(starts,
            std::vector<TG::NodePos> { {0, 0}, {0, 2}, {0, 4}, {0, 6} });
}

static void test_small_split() {
    auto g = TG::Graph::Builder()
        .add_node(TG::Str("aa"), "s1")
        .add_node(TG::Str("a"), "s2")
        .add_node(TG::Str("aa"), "s3")
        .add_node(TG::Str("a"), "s4")
        .add_edge("s1", "s2")
        .add_edge("s1", "s3")
        .add_edge("s2", "s4")
        .add_edge("s3", "s4")
        .build();

    auto ss = triegraph::SparseStarts<TG::Graph, TG::NodePos>(g);
    auto starts = ss.compute_starts_every(2, {0});

    std::ranges::equal(starts,
            std::vector<TG::NodePos> { {0, 0}, {1, 0}, {2, 0}, {3, 0} });
}

int main() {
    test_iter_concept();
    test_small_linear();
    test_small_split();

    return 0;
}
