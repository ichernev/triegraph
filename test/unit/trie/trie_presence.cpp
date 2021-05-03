#include "util/util.h"
#include "trie/trie_presence.h"
#include "trie/kmer.h"
#include "trie/dkmer.h"
#include "trie/kmer_settings.h"
#include "alphabet/letter.h"
#include "alphabet/dna_letter.h"

#include <vector>
#include <algorithm>

#include "testlib/test.h"

using triegraph::u8;
using triegraph::u16;
using triegraph::u32;
using triegraph::u64;

template<u64 nopts, u64 K,
    typename Letter_ = triegraph::Letter<u8, nopts>,
    bool allow_inner = false>
struct Helper {
    using Letter = Letter_;
    // using Kmer = triegraph::Kmer<Letter, u32, K>;
    using Kmer = triegraph::DKmer<Letter, u32>;
    using TB = triegraph::TriePresence<Kmer, allow_inner>;

    // struct Initializator {
    //     Initializator() {
    //         std::cerr << "initializing K " << K << std::endl;
    //         Kmer::setK(K);
    //     }
    // };
    // static inline Initializator init = {};
};

int m = test::define_module(__FILE__, [] {

test::define_test("no_inner_trie_presense", [] {
    using TB = Helper<4, 3, triegraph::dna::DnaLetter>::TB;
    TB::Kmer::set_settings(triegraph::KmerSettings::from_depth<u32>(3));

    auto kmers = std::vector<TB::Kmer> {
        TB::Kmer::from_str("aac"),
        TB::Kmer::from_str("aat"),
        TB::Kmer::from_str("ata"),
        TB::Kmer::from_str("cgt"),
        TB::Kmer::from_str("tat"),
    };

    auto tb = TB(kmers);
    // tb.init(kmers);

    assert( tb.contains(TB::Kmer::from_str("")));
    assert( tb.contains(TB::Kmer::from_str("a")));
    assert( tb.contains(TB::Kmer::from_str("aa")));
    assert(!tb.contains(TB::Kmer::from_str("ac")));
    assert(!tb.contains(TB::Kmer::from_str("ag")));
    assert( tb.contains(TB::Kmer::from_str("at")));
    assert( tb.contains(TB::Kmer::from_str("c")));
    assert(!tb.contains(TB::Kmer::from_str("ca")));
    assert(!tb.contains(TB::Kmer::from_str("cc")));
    assert( tb.contains(TB::Kmer::from_str("cg")));
    assert(!tb.contains(TB::Kmer::from_str("ct")));
    assert( tb.contains(TB::Kmer::from_str("t")));
    assert( tb.contains(TB::Kmer::from_str("ta")));
    assert(!tb.contains(TB::Kmer::from_str("tt")));
});

test::define_test("with_inner_trie_presense", [] {
    using TB = Helper<4, 3, triegraph::dna::DnaLetter, true>::TB;
    TB::Kmer::set_settings(triegraph::KmerSettings::from_depth<u32>(3));

    auto kmers = std::vector<TB::Kmer> {
        TB::Kmer::from_str("aac"),
        TB::Kmer::from_str("aat"),
        TB::Kmer::from_str("ata"),
        TB::Kmer::from_str("g"),
        TB::Kmer::from_str("cgt"),
        TB::Kmer::from_str("tat"),
        TB::Kmer::from_str("ta"),
    };

    auto tb = TB(kmers);
    // tb.init(kmers);

    assert( tb.contains(TB::Kmer::from_str("")));
    assert( tb.contains(TB::Kmer::from_str("a")));
    assert( tb.contains(TB::Kmer::from_str("aa")));
    assert(!tb.contains(TB::Kmer::from_str("ac")));
    assert(!tb.contains(TB::Kmer::from_str("ag")));
    assert( tb.contains(TB::Kmer::from_str("at")));
    assert( tb.contains(TB::Kmer::from_str("c")));
    assert(!tb.contains(TB::Kmer::from_str("ca")));
    assert(!tb.contains(TB::Kmer::from_str("cc")));
    assert( tb.contains(TB::Kmer::from_str("cg")));
    assert(!tb.contains(TB::Kmer::from_str("ct")));
    assert( tb.contains(TB::Kmer::from_str("g")));
    assert(!tb.contains(TB::Kmer::from_str("ga")));
    assert(!tb.contains(TB::Kmer::from_str("gc")));
    assert(!tb.contains(TB::Kmer::from_str("gg")));
    assert(!tb.contains(TB::Kmer::from_str("gt")));
    assert( tb.contains(TB::Kmer::from_str("t")));
    assert( tb.contains(TB::Kmer::from_str("ta")));
    assert(!tb.contains(TB::Kmer::from_str("tt")));
});

});
