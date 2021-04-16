#include "dna_config.h"
#include "manager.h"
#include "graph/complexity_component.h"
#include "graph/complexity_component_walker.h"

#include "helper.h"

using TG = triegraph::Manager<triegraph::dna::DnaConfig<0>>;
using ComplexityComponent = triegraph::ComplexityComponent<
    TG::Graph, TG::NodePos>;
using ComplexityComponentWalker = triegraph::ComplexityComponentWalker<
    ComplexityComponent>;

int m = test::define_module(__FILE__, [] {

test::define_test("all in cc", [] {
    auto graph = TG::Graph::Builder({
            .add_reverse_complement = false,
            .add_extends = false })
        .add_node(TG::Str("a"), "s1") /* disjoint */
        .add_node(TG::Str("a"), "s2") /* components */
        .build();

    auto ccw = ComplexityComponentWalker::Builder(graph, 2)
        .build(std::vector<TG::NodeLoc> {0, 1});

    assert(test::equal_sorted(
                ccw.cc_starts(graph, 2),
                std::vector<TG::NodePos> { {0, 0}, {1, 0} }));

    assert(test::equal_sorted(
                ccw.non_cc_starts(graph, 2),
                std::vector<TG::NodePos> { }));
});

test::define_test("none in cc", [] {
    auto graph = TG::Graph::Builder({
            .add_reverse_complement = false,
            .add_extends = false })
        .add_node(TG::Str("a"), "s1") /* disjoint */
        .add_node(TG::Str("a"), "s2") /* components */
        .build();

    auto ccw = ComplexityComponentWalker::Builder(graph, 2)
        .build(std::vector<TG::NodeLoc> {});

    assert(test::equal_sorted(
                ccw.cc_starts(graph, 2),
                std::vector<TG::NodePos> { }));

    assert(test::equal_sorted(
                ccw.non_cc_starts(graph, 2),
                std::vector<TG::NodePos> { {0, 0}, {1, 0} }));
});

test::define_test("simple incoming", [] {
    auto graph = TG::Graph::Builder({
            .add_reverse_complement = false,
            .add_extends = false })
        .add_node(TG::Str("aaaa"), "s1")
        .add_node(TG::Str("a"), "s2")
        .add_edge("s1", "s2")
        .build();

    auto ccw = ComplexityComponentWalker::Builder(graph, 2)
        .build(std::vector<TG::NodeLoc> {1});

    // for (auto const &p : test::sorted(ccw.cc_starts(graph, 2))) {
    //     std::cerr << p << std::endl;
    // }
    assert(test::equal_sorted(
                ccw.cc_starts(graph, 2),
                std::vector<TG::NodePos> { {0, 2}, {0, 3}, {1, 0} }));
    assert(test::equal_sorted(
                ccw.non_cc_starts(graph, 2),
                std::vector<TG::NodePos> { {0, 0}, {0, 1} }));
});

test::define_test("shared incoming", [] {
    auto graph = TG::Graph::Builder({
            .add_reverse_complement = false,
            .add_extends = false })
        .add_node(TG::Str("aaaa"), "s1") /* shared incoming edge */
        .add_node(TG::Str("a"), "s2")
        .add_node(TG::Str("a"), "s3")
        .add_edge("s1", "s2")
        .add_edge("s1", "s3")
        .build();

    auto ccw = ComplexityComponentWalker::Builder(graph, 2)
        .build(std::vector<TG::NodeLoc> {1, 2});

    // for (auto const &p : test::sorted(ccw.cc_starts(graph, 2))) {
    //     std::cerr << p << std::endl;
    // }
    assert(test::equal_sorted(
                ccw.cc_starts(graph, 2),
                std::vector<TG::NodePos> { {0, 2}, {0, 3}, {1, 0}, {2, 0} }));

    assert(test::equal_sorted(
                ccw.non_cc_starts(graph, 2),
                std::vector<TG::NodePos> { {0, 0}, {0, 1} }));
});

// check numbers with real CompEst

});
