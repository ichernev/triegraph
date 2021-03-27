#ifndef __DNA_CONFIG_H__
#define __DNA_CONFIG_H__

#include "util/util.h"
#include "alphabet/dna_letter.h"

namespace triegraph::dna {

using namespace triegraph;

template<u64 trie_depth = 15>
struct DnaConfig {
    using Letter = DnaLetter;
    using Letters = DnaLetters;
    using StrHolder = u32;
    using NodeLoc = u32;
    using NodeLen = u32;
    using EdgeLoc = u32;
    using LetterLoc = u32;
    using KmerHolder = u32;
    static constexpr bool triedata_allow_inner = false;
    static constexpr bool triedata_advanced = true;
    static constexpr int LetterLocIdxShift = 4;
    static constexpr u64 KmerLen = trie_depth;
    static constexpr KmerHolder on_mask = KmerHolder(1) << (
            sizeof(KmerHolder) * BITS_PER_BYTE - 1);
};

} /* namespace triedata::dna */

#endif /* __DNA_CONFIG_H__ */
