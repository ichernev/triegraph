#include "manager.h"
#include "dna_config.h"

#include "testlib/test.h"

using TG = triegraph::Manager<triegraph::dna::DnaConfig<0>>;

int m = test::define_module(__FILE__, [] {

test::define_test("simple linear", [] {
    auto graph = TG::Graph::Builder({
            .add_reverse_complement = false })
        .add_node(TG::Str("ac"), "s1")
        .add_node(TG::Str("gt"), "s2")
        .add_node(TG::Str("at"), "s3")
        .add_edge("s1", "s2")
        .add_edge("s2", "s3")
        .build();

    auto pairs = TG::pairs_from_graph<TG::TrieGraphBuilderNBFS>(
            graph,
            { .add_reverse_complement = false, .trie_depth = 2 },
            TG::Settings::NoSkip {});

    std::ranges::sort(pairs, [](const auto &a, const auto &b) {
            return a.second < b.second; });
    auto sr = std::ranges::unique(pairs);
    pairs.resize(sr.begin() - pairs.begin());

    // std::ranges::copy(pairs, std::ostream_iterator<TG::vec_pairs::value_type>(std::cerr, " "));
    // std::cerr << "----" << std::endl;
    // for (const auto &p : pairs) {
    //     std::cerr << p.first << " " << p.second << std::endl;
    // }
    // std::cerr << "----" << std::endl;
    assert(std::ranges::equal(pairs, TG::vec_pairs {
        { TG::Kmer::from_str("ac"), 2 },
        { TG::Kmer::from_str("cg"), 3 },
        { TG::Kmer::from_str("gt"), 4 },
        { TG::Kmer::from_str("ta"), 5 },
        { TG::Kmer::from_str("at"), 6 },
    }));
});

test::define_test("simple linear 2", [] {
    auto graph = TG::Graph::Builder({
            .add_reverse_complement = false })
        .add_node(TG::Str("ac"), "s1")
        .add_node(TG::Str("gt"), "s2")
        .add_node(TG::Str("at"), "s3")
        .add_edge("s1", "s2")
        .add_edge("s2", "s3")
        .build();

    // trie_depth = 3 will trigger second case in bfs
    auto pairs = TG::pairs_from_graph<TG::TrieGraphBuilderNBFS>(
            graph,
            { .add_reverse_complement = false, .trie_depth = 3 },
            TG::Settings::NoSkip {});

    std::ranges::sort(pairs, [](const auto &a, const auto &b) {
            return a.second < b.second; });
    auto sr = std::ranges::unique(pairs);
    pairs.resize(sr.begin() - pairs.begin());

    // std::ranges::copy(pairs, std::ostream_iterator<TG::vec_pairs::value_type>(std::cerr, " "));
    // std::cerr << "----" << std::endl;
    // for (const auto &p : pairs) {
    //     std::cerr << p.first << " " << p.second << std::endl;
    // }
    // std::cerr << "----" << std::endl;
    assert(std::ranges::equal(pairs, TG::vec_pairs {
        { TG::Kmer::from_str("acg"), 3 },
        { TG::Kmer::from_str("cgt"), 4 },
        { TG::Kmer::from_str("gta"), 5 },
        { TG::Kmer::from_str("tat"), 6 },
    }));
});

test::define_test("simple dag", [] {
    auto graph = TG::Graph::Builder({
            .add_reverse_complement = false })
        .add_node(TG::Str("a"), "s1")
        .add_node(TG::Str("cg"), "s2")
        .add_node(TG::Str("t"), "s3")
        .add_node(TG::Str("ac"), "s4")
        .add_edge("s1", "s2")
        .add_edge("s1", "s3")
        .add_edge("s2", "s4")
        .add_edge("s3", "s4")
        .build();

    auto pairs = TG::pairs_from_graph<TG::TrieGraphBuilderNBFS>(
            graph,
            { .add_reverse_complement = false, .trie_depth = 4 },
            TG::Settings::NoSkip {});

    std::ranges::sort(pairs);
    auto sr = std::ranges::unique(pairs);
    pairs.resize(sr.begin() - pairs.begin());

    assert(std::ranges::equal(pairs, TG::vec_pairs {
        { TG::Kmer::from_str("acga"), 5 },
        { TG::Kmer::from_str("atac"), 6 },
        { TG::Kmer::from_str("cgac"), 6 },
    }));
});

// test::define_test("simple cyclic", [] {
//     auto graph = TG::Graph::Builder({
//             .add_reverse_complement = false, .add_extends = false })
//         .add_node(TG::Str("a"), "s1")
//         .add_node(TG::Str("a"), "s2")
//         .add_edge("s1", "s2")
//         .add_edge("s2", "s1")
//         .build();

//     auto top_ord = triegraph::TopOrder<TG::Graph>::Builder(graph)
//         .build();

//     top_ord.sanity_check(graph, false);
//     assert(top_ord.idx[0] == 1);
//     assert(top_ord.idx[1] == 0);
// });

// test::define_test("simple cyclic 2", [] {
//     auto graph = TG::Graph::Builder({
//             .add_reverse_complement = false, .add_extends = false })
//         // define nodes in rev order to test if it properly finds the order
//         .add_node(TG::Str("a"), "s4")
//         .add_node(TG::Str("a"), "s3")
//         .add_node(TG::Str("a"), "s2")
//         .add_node(TG::Str("a"), "s1")
//         .add_edge("s1", "s2")
//         .add_edge("s2", "s3")
//         .add_edge("s3", "s2")
//         .add_edge("s3", "s4")
//         .build();

//     auto top_ord = triegraph::TopOrder<TG::Graph>::Builder(graph)
//         .build();

//     top_ord.sanity_check(graph, false);
//     assert(top_ord.idx[0] == 0);
//     assert(top_ord.idx[1] == 2);
//     assert(top_ord.idx[2] == 1);
//     assert(top_ord.idx[3] == 3);
// });

});
