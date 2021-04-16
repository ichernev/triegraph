#include "dna_config.h"
#include "manager.h"

#include <algorithm>
#include "helper.h"

using namespace triegraph;

using TG = Manager<dna::DnaConfig<0>>;

static std::vector<TG::LetterLoc> trie2graph(const TG::TrieGraphData &tg, TG::Kmer kmer) {
    auto pos_it = tg.trie_data.t2g_values_for(kmer);
    std::vector<TG::LetterLoc> pos;
    std::ranges::copy(pos_it, std::back_inserter(pos));
    std::sort(pos.begin(), pos.end());
    return pos;
}

static TG::TrieGraphData build_tgd(TG::Graph &&g) {
    return TG::triegraph_from_graph(
            std::move(g),
            {
                .add_reverse_complement = false,
                .trie_depth = 4,
                .algo = TG::Algo::BFS
            }).data;
}

int m = test::define_module(__FILE__, [] {

test::define_test("tiny_linear_graph", [] {
    auto g = TG::Graph::Builder({ .add_reverse_complement = false })
        .add_node(TG::Str("acgtacgtac"), "s1")
        .build();

    auto tg = build_tgd(std::move(g));
    auto pos = trie2graph(tg, TG::Kmer::from_str("acgt"));

    assert(std::ranges::equal(
                trie2graph(tg, TG::Kmer::from_str("acgt")),
                std::vector<TG::LetterLoc> { 4, 8 }));

    assert(std::ranges::equal(
                trie2graph(tg, TG::Kmer::from_str("cgta")),
                std::vector<TG::LetterLoc> { 5, 9 }));

    assert(std::ranges::equal(
                trie2graph(tg, TG::Kmer::from_str("gtac")),
                std::vector<TG::LetterLoc> { 6, 10 }));
});

test::define_test("small_nonlinear_graph", [] {
    auto g = TG::Graph::Builder({ .add_reverse_complement = false })
        .add_node(TG::Str("a"), "s1")
        .add_node(TG::Str("cg"), "s2")
        .add_node(TG::Str("t"), "s3")
        .add_node(TG::Str("ac"), "s4")
        .add_edge("s1", "s2")
        .add_edge("s1", "s3")
        .add_edge("s2", "s4")
        .add_edge("s3", "s4")
        .build();

    /****************
     *     12       *
     *     cg       *
     *  a /  \ ac   *
     *  0 \  / 45 6 *
     *     t        *
     *     3        *
     ***************/
    auto tg = build_tgd(std::move(g));
    assert(std::ranges::equal(
                trie2graph(tg, TG::Kmer::from_str("acga")),
                std::vector<TG::LetterLoc> { 5 }));
    assert(std::ranges::equal(
                trie2graph(tg, TG::Kmer::from_str("atac")),
                std::vector<TG::LetterLoc> { 6 }));
    assert(std::ranges::equal(
                trie2graph(tg, TG::Kmer::from_str("cgac")),
                std::vector<TG::LetterLoc> { 6 }));

    // std::cerr << tg.trie_data.active_trie.size() << std::endl;
    // for (const auto &kmer : tg.trie_data.active_trie) {
    //     std::cerr << "at " << kmer << std::endl;;
    // }
    assert(tg.trie_data.trie_inner_contains(TG::Kmer::from_str("acg")));
    assert(tg.trie_data.trie_inner_contains(TG::Kmer::from_str("cga")));
    assert(tg.trie_data.trie_inner_contains(TG::Kmer::from_str("ata")));
    assert(tg.trie_data.trie_inner_contains(TG::Kmer::from_str("cga")));
});

test::define_test("multiple_ends", [] {
    auto g = TG::Graph::Builder({ .add_reverse_complement = false })
        .add_node(TG::Str("acg"), "s1")
        .add_node(TG::Str("c"), "s2")
        .add_node(TG::Str("g"), "s3")
        .add_edge("s1", "s2")
        .add_edge("s1", "s3")
        .build();

    /*****************
     *       3   5   *
     *       c - a   *
     *  acg /        *
     *  012 \        *
     *       g - a   *
     *       4   6   *
     *****************/
    auto tg = build_tgd(std::move(g));

    // for (const auto &x : tg.trie_data.trie2graph) {
    //     std::cerr << x.first << "|" << TG::Kmer::from_compressed_leaf(x.first)
    //         << " : " << x.second << std::endl;
    // }

    // finishing at both ends should work
    assert(std::ranges::equal(
                trie2graph(tg, TG::Kmer::from_str("acgc")),
                std::vector<TG::LetterLoc> { 5 }));

    assert(std::ranges::equal(
                trie2graph(tg, TG::Kmer::from_str("acgg")),
                std::vector<TG::LetterLoc> { 6 }));

    // finishing at dummy extends should not work
    assert(std::ranges::equal(
                trie2graph(tg, TG::Kmer::from_str("cgca")),
                std::vector<TG::LetterLoc> { }));

    assert(std::ranges::equal(
                trie2graph(tg, TG::Kmer::from_str("cgga")),
                std::vector<TG::LetterLoc> { }));
});

});
