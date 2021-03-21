#include "util/util.h"
#include "trie/trie_data_opt.h"
#include "trie/kmer.h"
#include "alphabet/letter.h"
#include "alphabet/dna_letter.h"

#include <vector>
#include <algorithm>

#include <assert.h>

using triegraph::u8;
using triegraph::u16;
using triegraph::u32;
using triegraph::u64;

template<u64 nopts, u64 K,
    typename Letter_ = triegraph::Letter<u8, nopts, char, void, void>>
struct Helper {
    using Letter = Letter_;
    using Kmer = triegraph::Kmer<Letter, u32, K>;
    using TB = triegraph::TieredBitset<Kmer>;
};

static void test_trie_presense() {
    using TB = Helper<4, 3, triegraph::dna::DnaLetter>::TB;

    auto kmers = std::vector<TB::Kmer> {
        TB::Kmer::from_str("aac"),
        TB::Kmer::from_str("aat"),
        TB::Kmer::from_str("ata"),
        TB::Kmer::from_str("cgt"),
        TB::Kmer::from_str("tat"),
    };

    auto tb = TB();
    tb.init(kmers);

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
}

int main() {
    test_trie_presense();

    return 0;
}
