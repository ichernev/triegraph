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

static TG::TrieGraphData build_tgd(TG::Graph &&g,
        u32 start_every = 1, u32 cut_early_threshold = 500) {
    return TG::triegraph_from_graph(
            std::move(g),
            {
                .add_reverse_complement = false,
                .trie_depth = 4,
                .algo = TG::Settings::POINT_BFS,
                .skip_every = start_every,
                .cut_early_threshold = cut_early_threshold,
            }).data;
}

struct Cfg : public dna::DnaConfig<0> {
    static constexpr bool triedata_allow_inner = true;
};

int m = test::define_module(__FILE__, [] {

test::define_test("all_tiny_linear_graph", [] {
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

test::define_test("all_small_nonlinear_graph", [] {
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

test::define_test("all_multiple_ends", [] {
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

test::define_test("se2_tiny_linear_graph", [] {
    auto g = TG::Graph::Builder({ .add_reverse_complement = false })
        .add_node(TG::Str("acgtacgtac"), "s1")
        .build();

    auto tg = build_tgd(std::move(g), 2);
    auto pos = trie2graph(tg, TG::Kmer::from_str("acgt"));

    assert(std::ranges::equal(
                trie2graph(tg, TG::Kmer::from_str("acgt")),
                std::vector<TG::LetterLoc> { 4, 8 }));

    // none, it's not started from odd positions
    assert(std::ranges::equal(
                trie2graph(tg, TG::Kmer::from_str("cgta")),
                std::vector<TG::LetterLoc> { }));

    assert(std::ranges::equal(
                trie2graph(tg, TG::Kmer::from_str("gtac")),
                std::vector<TG::LetterLoc> { 6, 10 }));
});
test::define_test("cut_early", [] {
// static void test_cut_early() {
    using TGX = Manager<Cfg>;
    auto kmer_s = &TGX::Kmer::from_str;
    auto g = TGX::Graph::Builder({ .add_reverse_complement = false })
        .add_node(TGX::Str("a"), "s00")
        .add_node(TGX::Str("g"), "s01")
        .add_node(TGX::Str("c"), "s10")
        .add_node(TGX::Str("t"), "s11")
        .add_node(TGX::Str("a"), "s20")
        .add_node(TGX::Str("g"), "s21")
        .add_node(TGX::Str("c"), "s30")
        .add_node(TGX::Str("t"), "s31")
        .add_node(TGX::Str("a"), "s40")
        .add_node(TGX::Str("g"), "s41")
        .add_edge("s00", "s10")
        .add_edge("s00", "s11")
        .add_edge("s01", "s10")
        .add_edge("s01", "s11")
        .add_edge("s10", "s20")
        .add_edge("s10", "s21")
        .add_edge("s11", "s20")
        .add_edge("s11", "s21")
        .add_edge("s20", "s30")
        .add_edge("s20", "s31")
        .add_edge("s21", "s30")
        .add_edge("s21", "s31")
        .add_edge("s30", "s40")
        .add_edge("s30", "s41")
        .add_edge("s31", "s40")
        .add_edge("s31", "s41")
        .build();

    auto tg = TGX::triegraph_from_graph(
            std::move(g),
            {
                .add_reverse_complement = false,
                .trie_depth = 4,
                .algo = TGX::Settings::POINT_BFS,
                .skip_every = 100,
                .cut_early_threshold = 8,
            }).data.trie_data;

    // std::cerr << "in T2G" << std::endl;
    // for (const auto &p : tg.trie2graph) {
    //     std::cerr << p << " " << TGX::Kmer::from_compressed(p.first) << std::endl;
    // }

    assert(!tg.trie_contains(kmer_s("acac")));
    assert( tg.trie_contains(kmer_s("aca")));
    assert( tg.trie_contains(kmer_s("ata")));
});

});
