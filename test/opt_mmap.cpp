#include "manager.h"
#include "dna_config.h"

#include <vector>
#include <utility>

using TG = triegraph::Manager<triegraph::dna::DnaConfig<0>>;

using t2g_t = triegraph::OptMMap<TG::Kmer::Holder, triegraph::u32, TG::LetterLoc>;

static void test_init_iter() {
    TG::init({ .trie_depth = 2 });
    t2g_t t2g;
    auto pairs = std::vector<std::pair<TG::Kmer, TG::LetterLoc>> {
        { TG::Kmer::from_str("aa"), 0 },
        { TG::Kmer::from_str("aa"), 1 },
        { TG::Kmer::from_str("ac"), 1 },
        { TG::Kmer::from_str("ac"), 2 },
        { TG::Kmer::from_str("ta"), 3 },
    };

    auto a_max = triegraph::pow(TG::Kmer::K, TG::Letter::num_options);
    // std::cerr << "a_max " << a_max << std::endl;
    t2g.template init<
        triegraph::PairFwd<TG::Kmer, TG::LetterLoc>,
        triegraph::CookKmer<TG::Kmer>>(
            pairs,
            a_max);

    assert(pairs.size() == t2g.size());
    assert((pairs.end() - pairs.begin()) == (ssize_t) t2g.size());
    int i = 0;
    // std::cerr << "starts:" << std::endl;
    // std::copy(t2g.start.begin(), t2g.start.end(),
    //         std::ostream_iterator<triegraph::u32>(std::cerr, " "));
    // std::cerr << std::endl;
    // std::cerr << "elems:" << std::endl;
    // std::copy(t2g.elems.begin(), t2g.elems.end(),
    //         std::ostream_iterator<triegraph::u32>(std::cerr, " "));
    // std::cerr << std::endl;
    // std::cerr << "-------" << std::endl;
    for (auto p : t2g) {
        assert(p.first == pairs[i].first.compress_leaf());
        assert(p.second == pairs[i].second);
        // std::cerr << p.first << " " << p.second << std::endl;

        ++i;
    }
}

static void test_equal_range() {
    TG::init({ .trie_depth = 2 });
    t2g_t t2g;
    auto er = [&](auto str) { return t2g.equal_range(TG::Kmer::from_str(str).compress_leaf()); };
    auto pairs = std::vector<std::pair<TG::Kmer, TG::LetterLoc>> {
        { TG::Kmer::from_str("aa"), 0 },
        { TG::Kmer::from_str("aa"), 1 },
        { TG::Kmer::from_str("ac"), 1 },
        { TG::Kmer::from_str("ac"), 2 },
        { TG::Kmer::from_str("ta"), 3 },
    };

    auto a_max = triegraph::pow(TG::Kmer::K, TG::Letter::num_options);
    t2g.template init<triegraph::PairFwd<TG::Kmer, TG::LetterLoc>>(
            pairs,
            a_max);

    auto rng_aa = er("aa");
    auto rng_aa_expected = std::vector<decltype(rng_aa.first)::value_type> {
        { 0, 0 },
        { 0, 1 },
    };
    assert(std::equal(rng_aa.first, rng_aa.second, rng_aa_expected.begin()));

    auto rng_ac = er("ac");
    auto rng_ac_expected = std::vector<std::pair<TG::Kmer::Holder, TG::LetterLoc>> {
        { 1, 1 },
        { 1, 2 },
    };
    assert(std::equal(rng_ac.first, rng_ac.second, rng_ac_expected.begin()));

    auto rng_ta = er("ta");
    auto rng_ta_expected = std::vector<std::pair<TG::Kmer::Holder, TG::LetterLoc>> {
        { 12, 3 },
    };
    assert(std::equal(rng_ta.first, rng_ta.second, rng_ta_expected.begin()));

    auto rng_tt = er("tt");
    auto rng_tt_expected = std::vector<std::pair<TG::Kmer::Holder, TG::LetterLoc>> {};
    assert(std::equal(rng_tt.first, rng_tt.second, rng_tt_expected.begin()));
}


static void test_iter_concept() {
    TG::init({ .trie_depth = 2 });

    auto func = [](std::forward_iterator auto it) {};

    t2g_t t2g;
    auto pairs = std::vector<std::pair<TG::Kmer, TG::LetterLoc>> {
        { TG::Kmer::from_str("aa"), 0 },
        { TG::Kmer::from_str("aa"), 1 },
        { TG::Kmer::from_str("ac"), 1 },
        { TG::Kmer::from_str("ac"), 2 },
        { TG::Kmer::from_str("ta"), 3 },
    };

    auto a_max = triegraph::pow(TG::Kmer::K, TG::Letter::num_options);
    t2g.template init<triegraph::PairFwd<TG::Kmer, TG::LetterLoc>>(
            pairs,
            a_max);

    func(t2g.begin());
    func(t2g.end());
    func(t2g.equal_range(TG::Kmer::from_str("aa").compress_leaf()).first);
    func(t2g.equal_range(TG::Kmer::from_str("aa").compress_leaf()).second);
}

int main() {
    test_init_iter();
    test_equal_range();
    test_iter_concept();

    return 0;
}
