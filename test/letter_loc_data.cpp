#include "dna_config.h"
#include "manager.h"

#include <utility>
#include <vector>
#include <assert.h>

struct LLNoIdx : public triegraph::dna::DnaConfig<0> {
    static constexpr int LetterLocIdxShift = -1;
};

struct LLWithIdx : public triegraph::dna::DnaConfig<0> {
    static constexpr int LetterLocIdxShift = 2;
};

using TG = triegraph::Manager<LLNoIdx>;

static TG::Graph build_graph() {
    TG::init();
    return TG::Graph::Builder()
        .add_node(TG::Str("acgt"), "s1")
        .add_node(TG::Str("a"), "s2")
        .add_node(TG::Str("cgt"), "s3")
        .add_node(TG::Str("aa"), "s4")
        .add_edge("s1", "s2")
        .add_edge("s1", "s3")
        .add_edge("s2", "s4")
        .add_edge("s3", "s4")
        .build({ .add_reverse_complement = false });

     /*******************
      *       [1]       *
      *  [0]  A   [3][4]*
      *  ACGT/4  \ AA-a *
      *  0123\[2]/ 89 9 *
      *       CGT       *
      *       567       *
      ******************/
}

static std::vector<std::pair<TG::LetterLoc, TG::NodePos>> get_expected() {
    return std::vector<std::pair<TG::LetterLoc, TG::NodePos>> {
        { 0,  { 0,  0 } },
        { 1,  { 0,  1 } },
        { 2,  { 0,  2 } },
        { 3,  { 0,  3 } },
        { 4,  { 1,  0 } },
        { 5,  { 2,  0 } },
        { 6,  { 2,  1 } },
        { 7,  { 2,  2 } },
        { 8,  { 3,  0 } },
        { 9,  { 3,  1 } },
        { 10, { 4,  0 } },
    };
}

static void test_lloc_no_idx() {
    auto g = build_graph();
    auto ll = TG::LetterLocData(g);
    auto expected = get_expected();

    assert(ll.num_locations == 11);

    for (auto e : expected) {
        // std::cerr << e.first << " " << e.second << std::endl;
        // std::cerr << ll.expand(e.first) << " " << ll.compress(e.second) << std::endl;
        assert(ll.expand(e.first) == e.second);
        assert(ll.compress(e.second) == e.first);
    }
}

static void test_lloc_with_idx() {
    using TG2 = triegraph::Manager<LLWithIdx>;

    auto g = build_graph();
    auto ll = TG2::LetterLocData(g);
    auto expected = get_expected();

    assert(ll.num_locations == 11);

    for (auto e : expected) {
        // std::cerr << e.first << " " << e.second << std::endl;
        // std::cerr << ll.expand(e.first) << " " << ll.compress(e.second) << std::endl;
        assert(ll.expand(e.first) == e.second);
        assert(ll.compress(e.second) == e.first);
    }
}


int main() {
    test_lloc_no_idx();
    test_lloc_with_idx();

    return 0;
}
