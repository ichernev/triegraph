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

test::define_test("simple cc_iter", [] {
    auto graph = TG::Graph::Builder({
            .add_reverse_complement = false,
            .add_extends = false })
        .add_node(TG::Str("a"), "s1") /* disjoint */
        .add_node(TG::Str("a"), "s2") /* components */
        .build();

    auto ccw = ComplexityComponentWalker::Builder(
            graph,
            std::vector<TG::NodeLoc> {0, 1},
            2).build();

    assert(std::ranges::equal(
                ccw.cc_starts(graph, 2),
                std::vector<TG::NodePos> { {0, 0}, {1, 0} }));
});

test::define_test("simple non_cc_iter", [] {
    auto graph = TG::Graph::Builder({
            .add_reverse_complement = false,
            .add_extends = false })
        .add_node(TG::Str("a"), "s1") /* disjoint */
        .add_node(TG::Str("a"), "s2") /* components */
        .build();

    auto ccw = ComplexityComponentWalker::Builder(
            graph,
            std::vector<TG::NodeLoc> {},
            2).build();

    assert(std::ranges::equal(
                ccw.non_cc_starts(graph, 2),
                std::vector<TG::NodePos> { {0, 0}, {1, 0} }));
});

});
