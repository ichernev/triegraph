#include "manager.h"
#include "dna_config.h"

#include <vector>
#include <utility>

using TG = triegraph::Manager<triegraph::dna::DnaConfig<0>>;

using t2g_t = triegraph::OptMMap<TG::Kmer::Holder, triegraph::u32, TG::LetterLoc>;

// ni -- no-inner (only leaves)
static void test_ni_init_iter() {
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

// wi -- with-inner (allow any trie elem to be key)
static void test_wi_init_iter() {
    TG::init({ .trie_depth = 2 });
    t2g_t t2g;
    auto pairs = std::vector<std::pair<TG::Kmer, TG::LetterLoc>> {
        { TG::Kmer::from_str("c"),  3 },
        { TG::Kmer::from_str("t"),  4 },
        { TG::Kmer::from_str("aa"), 0 },
        { TG::Kmer::from_str("aa"), 1 },
        { TG::Kmer::from_str("ac"), 1 },
        { TG::Kmer::from_str("ac"), 2 },
        { TG::Kmer::from_str("ta"), 5 },
    };

    auto a_max = TG::Kmer::NUM_COMPRESSED;
    // std::cerr << "a_max " << a_max << std::endl;
    t2g.template init<
        triegraph::PairFwd<TG::Kmer, TG::LetterLoc>,
        triegraph::CookKmer<TG::Kmer, true>>(
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
        assert(p.first == pairs[i].first.compress());
        assert(p.second == pairs[i].second);
        // std::cerr << p.first << " " << p.second << std::endl;

        ++i;
    }
}

// ni -- no-inner (only leaves)
static void test_ni_equal_range() {
    TG::init({ .trie_depth = 2 });
    t2g_t t2g;
    auto er = [&](auto str) {
        auto er = t2g.equal_range(TG::Kmer::from_str(str).compress_leaf());
        return triegraph::iter_pair(er.first, er.second);
    };
    auto pairs = std::vector<std::pair<TG::Kmer, TG::LetterLoc>> {
        { TG::Kmer::from_str("aa"), 0 },
        { TG::Kmer::from_str("aa"), 1 },
        { TG::Kmer::from_str("ac"), 1 },
        { TG::Kmer::from_str("ac"), 2 },
        { TG::Kmer::from_str("ta"), 3 },
    };

    auto a_max = TG::Kmer::NUM_LEAFS;
    t2g.template init<triegraph::PairFwd<TG::Kmer, TG::LetterLoc>>(
            pairs,
            a_max);

    using vector_t = std::vector<std::pair<TG::Kmer::Holder, TG::LetterLoc>>;

    assert(std::ranges::equal(er("aa"), vector_t { {0, 0}, {0, 1} }));
    assert(std::ranges::equal(er("ac"), vector_t { {1, 1}, {1, 2} }));
    assert(std::ranges::equal(er("ta"), vector_t { {12, 3} }));
    assert(std::ranges::equal(er("tt"), vector_t { }));
}

static void test_wi_equal_range() {
    TG::init({ .trie_depth = 2 });
    t2g_t t2g;
    auto er = [&](auto str) {
        auto er = t2g.equal_range(TG::Kmer::from_str(str).compress());
        return triegraph::iter_pair(er.first, er.second);
    };
    auto pairs = std::vector<std::pair<TG::Kmer, TG::LetterLoc>> {
        { TG::Kmer::from_str("c"),  3 },
        { TG::Kmer::from_str("t"),  4 },
        { TG::Kmer::from_str("aa"), 0 },
        { TG::Kmer::from_str("aa"), 1 },
        { TG::Kmer::from_str("ac"), 1 },
        { TG::Kmer::from_str("ac"), 2 },
        { TG::Kmer::from_str("ta"), 5 },
        { TG::Kmer::from_str("tt"), 6 },
    };

    auto a_max = TG::Kmer::NUM_COMPRESSED;
    t2g.template init<
        triegraph::PairFwd<TG::Kmer, TG::LetterLoc>,
        triegraph::CookKmer<TG::Kmer, true>>(
            pairs,
            a_max);

    using vector_t = std::vector<std::pair<TG::Kmer::Holder, TG::LetterLoc>>;

    assert(std::ranges::equal(er("c"), vector_t { {2, 3} }));
    assert(std::ranges::equal(er("g"), vector_t { }));
    assert(std::ranges::equal(er("aa"), vector_t { {5, 0}, {5, 1} }));
    assert(std::ranges::equal(er("ac"), vector_t { {6, 1}, {6, 2} }));
    assert(std::ranges::equal(er("ta"), vector_t { {17, 5} }));
    assert(std::ranges::equal(er("tt"), vector_t { {20, 6} }));
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
    test_ni_init_iter();
    test_wi_init_iter();

    test_ni_equal_range();
    test_wi_equal_range();

    test_iter_concept();

    return 0;
}
