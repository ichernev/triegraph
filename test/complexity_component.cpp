#include "dna_config.h"
#include "manager.h"
#include "graph/complexity_component.h"

#include "helper.h"


using TG = triegraph::Manager<triegraph::dna::DnaConfig<0>>;
using ComplexityComponent = triegraph::ComplexityComponent<
    TG::Graph, TG::NodePos>;

static void test_cc(auto &cc,
        std::vector<TG::NodeLoc> inc,
        std::vector<TG::NodeLoc> out,
        std::vector<TG::NodeLoc> internal) {

    assert(std::ranges::equal(cc.incoming, inc));
    assert(std::ranges::equal(cc.outgoing, out));
    std::ranges::sort(cc.internal);
    assert(std::ranges::equal(cc.internal, internal));
}

static void test_cc_iter(ComplexityComponent &cc, TG::Graph &graph, triegraph::u32 trie_depth,
        std::vector<TG::NodePos> expected) {
    auto fn_range_check = [] (std::ranges::range auto const& r) {};
    fn_range_check(cc.starts_inside(graph, trie_depth));

    auto starts = cc.starts_inside(graph, trie_depth);
    std::vector<TG::NodePos> actual;
    std::ranges::copy(starts, std::back_inserter(actual));
    std::ranges::sort(actual, [](const auto &a, const auto &b) {
        return a.node != b.node ? a.node < b.node : a.pos < b.pos;
    });
    assert(std::ranges::equal(actual, expected));
}

int m = test::define_module(__FILE__, [] {

test::define_test("all short", [] {
    auto graph = TG::Graph::Builder({
            .add_reverse_complement = false,
            .add_extends = false })
        .add_node(TG::Str("a"), "s1")
        .add_node(TG::Str("a"), "s2")
        .add_node(TG::Str("a"), "s3")
        .add_edge("s1", "s2")
        .add_edge("s2", "s3")
        .build();

    {
        auto cc = ComplexityComponent::Builder(graph, 0, 2).build();
        test_cc(cc, {}, {}, {0, 1, 2});
        test_cc_iter(cc, graph, 2, { {0, 0}, {1, 0}, {2, 0} });
    }

    {
        auto cc = ComplexityComponent::Builder(graph, 1, 2).build();
        test_cc(cc, {}, {}, {0, 1, 2});
        test_cc_iter(cc, graph, 2, { {0, 0}, {1, 0}, {2, 0} });
    }

    {
        auto cc = ComplexityComponent::Builder(graph, 2, 2).build();
        test_cc(cc, {}, {}, {0, 1, 2});
        test_cc_iter(cc, graph, 2, { {0, 0}, {1, 0}, {2, 0} });
    }
});

test::define_test("all short loop", [] {
    auto graph = TG::Graph::Builder({
            .add_reverse_complement = false,
            .add_extends = false })
        .add_node(TG::Str("a"), "s1")
        .add_node(TG::Str("a"), "s2")
        .add_node(TG::Str("a"), "s3")
        .add_edge("s1", "s2")
        .add_edge("s2", "s3")
        .add_edge("s3", "s1")
        .build();

    {
        auto cc = ComplexityComponent::Builder(graph, 0, 2).build();
        test_cc(cc, {}, {}, {0, 1, 2});
        test_cc_iter(cc, graph, 2, { {0, 0}, {1, 0}, {2, 0} });
    }

    {
        auto cc = ComplexityComponent::Builder(graph, 1, 2).build();
        test_cc(cc, {}, {}, {0, 1, 2});
        test_cc_iter(cc, graph, 2, { {0, 0}, {1, 0}, {2, 0} });
    }

    {
        auto cc = ComplexityComponent::Builder(graph, 2, 2).build();
        test_cc(cc, {}, {}, {0, 1, 2});
        test_cc_iter(cc, graph, 2, { {0, 0}, {1, 0}, {2, 0} });
    }
});

test::define_test("incoming only", [] {
    auto graph = TG::Graph::Builder({
            .add_reverse_complement = false,
            .add_extends = false })
        .add_node(TG::Str("aaa"), "s1")
        .add_node(TG::Str("a"), "s2")
        .add_node(TG::Str("a"), "s3")
        .add_edge("s1", "s2")
        .add_edge("s2", "s3")
        .build();

    {
        auto cc = ComplexityComponent::Builder(graph, 1, 2).build();
        test_cc(cc, {0}, {}, {1, 2});
        test_cc_iter(cc, graph, 2, { {0, 1}, {0, 2}, {1, 0}, {2, 0} });
    }

    {
        auto cc = ComplexityComponent::Builder(graph, 2, 2).build();
        test_cc(cc, {0}, {}, {1, 2});
        test_cc_iter(cc, graph, 2, { {0, 1}, {0, 2}, {1, 0}, {2, 0} });
    }
});

test::define_test("outgoing only", [] {
    auto graph = TG::Graph::Builder({
            .add_reverse_complement = false,
            .add_extends = false })
        .add_node(TG::Str("a"), "s1")
        .add_node(TG::Str("a"), "s2")
        .add_node(TG::Str("aa"), "s3")
        .add_edge("s1", "s2")
        .add_edge("s2", "s3")
        .build();

    {
        auto cc = ComplexityComponent::Builder(graph, 0, 2).build();
        test_cc(cc, {}, {2}, {0, 1});
        test_cc_iter(cc, graph, 2, { {0, 0}, {1, 0} });
    }

    {
        auto cc = ComplexityComponent::Builder(graph, 1, 2).build();
        test_cc(cc, {}, {2}, {0, 1});
        test_cc_iter(cc, graph, 2, { {0, 0}, {1, 0} });
    }
});

test::define_test("in/out shared", [] {
    auto graph = TG::Graph::Builder({
            .add_reverse_complement = false,
            .add_extends = false })
        .add_node(TG::Str("a"), "s1")
        .add_node(TG::Str("a"), "s2")
        .add_node(TG::Str("aaa"), "s3") /* both incoming and outgoing */
        .add_node(TG::Str("a"), "s4")
        .add_edge("s1", "s2")
        .add_edge("s1", "s3")
        .add_edge("s2", "s4")
        .add_edge("s3", "s4")
        .build();

    {
        auto cc = ComplexityComponent::Builder(graph, 0, 2).build();
        test_cc(cc, {2}, {2}, {0, 1, 3});
        test_cc_iter(cc, graph, 2, { {0, 0}, {1, 0}, {2, 1}, {2, 2}, {3, 0} });
    }

    {
        auto cc = ComplexityComponent::Builder(graph, 1, 2).build();
        test_cc(cc, {2}, {2}, {0, 1, 3});
        test_cc_iter(cc, graph, 2, { {0, 0}, {1, 0}, {2, 1}, {2, 2}, {3, 0} });
    }

    {
        auto cc = ComplexityComponent::Builder(graph, 3, 2).build();
        test_cc(cc, {2}, {2}, {0, 1, 3});
        test_cc_iter(cc, graph, 2, { {0, 0}, {1, 0}, {2, 1}, {2, 2}, {3, 0} });
    }
});

});
