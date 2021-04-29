#include "testlib/test.h"

#include "dna_config.h"
#include "manager.h"

using namespace triegraph;

using TG = Manager<dna::DnaConfig<0, false, true>>;

int m = test::define_module(__FILE__, [] {

test::define_test("simple linear", [] {
    // TG::init is only needed for DKmer initialization
    // TG::init({ .trie_depth = 4 });
    auto graph = TG::Graph::Builder({
            .add_reverse_complement = false,
            .add_extends = false })
        .add_node(TG::Str("acgt"), "s1")
        .add_node(TG::Str("acgt"), "s2")
        .add_node(TG::Str("acgt"), "s3")
        .add_edge("s1", "s2")
        .add_edge("s2", "s3")
        .build();

    auto top_ord = TG::TopOrder::Builder(graph)
        .build();

    auto ce = TG::ComplexityEstimator(graph, top_ord, 4, 1, 1);
    ce.compute();
    assert(std::ranges::equal(ce.get_starts(), std::vector<TG::KmerHolder> { 1, 1, 1 }));
    assert(std::ranges::equal(ce.get_ends(), std::vector<TG::KmerHolder> { 1, 1, 1 }));
});

test::define_test("simple dag", [] {
    // TG::init is only needed for DKmer initialization
    // TG::init({ .trie_depth = 4 });
    auto graph = TG::Graph::Builder({
            .add_reverse_complement = false,
            .add_extends = false })
        .add_node(TG::Str("acgt"), "s1")
        .add_node(TG::Str("acgt"), "s2")
        .add_node(TG::Str("acgt"), "s3")
        .add_node(TG::Str("acgt"), "s4")
        .add_edge("s1", "s2")
        .add_edge("s1", "s3")
        .add_edge("s2", "s4")
        .add_edge("s3", "s4")
        .build();

    auto top_ord = TG::TopOrder::Builder(graph)
        .build();

    auto ce = TG::ComplexityEstimator(graph, top_ord, 4, 1, 1);
    ce.compute();
    assert(std::ranges::equal(ce.get_starts(), std::vector<TG::KmerHolder> { 1, 1, 1, 2 }));
    assert(std::ranges::equal(ce.get_ends(), std::vector<TG::KmerHolder> { 1, 1, 1, 1 }));
});

test::define_test("simple dag2", [] {
    // TG::init is only needed for DKmer initialization
    // TG::init({ .trie_depth = 4 });
    auto graph = TG::Graph::Builder({
            .add_reverse_complement = false,
            .add_extends = false })
        .add_node(TG::Str("a"), "s00")
        .add_node(TG::Str("a"), "s01")
        .add_node(TG::Str("a"), "s10")
        .add_node(TG::Str("a"), "s11")
        .add_node(TG::Str("a"), "s20")
        .add_node(TG::Str("a"), "s21")
        .add_node(TG::Str("a"), "s3")
        .add_edge("s00", "s10")
        .add_edge("s00", "s11")
        .add_edge("s01", "s10")
        .add_edge("s01", "s11")
        .add_edge("s10", "s20")
        .add_edge("s10", "s21")
        .add_edge("s11", "s20")
        .add_edge("s11", "s21")
        .add_edge("s20", "s3")
        .add_edge("s21", "s3")
        .build();

    auto top_ord = TG::TopOrder::Builder(graph)
        .build();

    auto ce = TG::ComplexityEstimator(graph, top_ord, 4, 1, 1);
    ce.compute();
    assert(std::ranges::equal(ce.get_starts(),
                std::vector<TG::KmerHolder> { 1, 1, 2, 2, 4, 4, 8 }));
    assert(std::ranges::equal(ce.get_ends(),
                std::vector<TG::KmerHolder> { 1, 1, 2, 2, 4, 4, 8 }));
});

test::define_test("simple loop", [] {
    // TG::init is only needed for DKmer initialization
    // TG::init({ .trie_depth = 4 });
    auto graph = TG::Graph::Builder({
            .add_reverse_complement = false,
            .add_extends = false })
        .add_node(TG::Str("a"), "s1")
        .add_node(TG::Str("a"), "s2")
        .add_edge("s1", "s2")
        .add_edge("s2", "s2")
        .build();

    auto top_ord = TG::TopOrder::Builder(graph)
        .build();

    auto ce = TG::ComplexityEstimator(graph, top_ord, 4, 1, 1);
    ce.compute();
    // std::ranges::copy(ce.get_starts(), std::ostream_iterator<TG::KmerHolder>(std::cerr, " ")); std::cerr << std::endl;
    // std::ranges::copy(ce.get_ends(), std::ostream_iterator<TG::KmerHolder>(std::cerr, " ")); std::cerr << std::endl;
    assert(std::ranges::equal(ce.get_starts(),
                std::vector<TG::KmerHolder> { 1, pow(4, 3) + 1 }));
    assert(std::ranges::equal(ce.get_ends(),
                std::vector<TG::KmerHolder> { 1, pow(4, 3) }));
});


});
