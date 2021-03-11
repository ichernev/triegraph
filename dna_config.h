#ifndef __DNA_CONFIG_H__
#define __DNA_CONFIG_H__

#include "util.h"
#include "dna_letter.h"

namespace triegraph::dna {

using namespace triegraph;

template<u64 trie_depth = 31>
struct DnaConfig {
    using Letter = DnaLetter;
    using Letters = DnaLetters;
    using StrHolder = u32;
    using NodeLoc = u32;
    using NodeLen = u32;
    using EdgeLoc = u32;
    using LetterLoc = u32;
    using KmerHolder = u64;
    static constexpr u64 KmerLen = trie_depth;
    static constexpr KmerHolder on_mask = KmerHolder(1) << 63;
};

} /* namespace triedata::dna */

#endif /* __DNA_CONFIG_H__ */
