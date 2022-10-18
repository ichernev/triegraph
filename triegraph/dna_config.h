// SPDX-License-Identifier: MPL-2.0
/*
 * Copyright (c) 2021, Iskren Chernev
 */

#ifndef __DNA_CONFIG_H__
#define __DNA_CONFIG_H__

#include "triegraph/util/util.h"
#include "triegraph/util/vector_pairs.h"
#include "triegraph/alphabet/dna_letter.h"

namespace triegraph::dna {

using namespace triegraph;

struct CfgFlags {
    /** enable using shorter-than-full kmers (only POINT_BFS supports them) */
    static constexpr u32 ALLOW_INNER_KMER = 1u << 0;
    /** use Kmer/DKmer type in VectorPairs instead of compressed/_leaf kmers.
     * Mostly useful during tests of Algo, where equality checks are performed
     * expecting Kmer type and not an integer */
    static constexpr u32 VP_RAW_KMERS     = 1u << 1;
    /** Use 2 separate vectors in VectorPairs instead of a single vector of
     * pairs. Necessary for zero-overhead */
    static constexpr u32 VP_DUAL_IMPL     = 1u << 2;
    /** Use SortedVector as a storage for TD instead of regular vector -- uses
     * around 1/8-1/4 of the memory */
    static constexpr u32 TD_SORTED_VECTOR = 1u << 3;
    /** Build TrieData from VectorPairs without allocating more memory than
     * necessary. Requires SortedVector, Dual Impl and DenseMMap (default) */
    static constexpr u32 TD_ZERO_OVERHEAD = 1u << 4;
    /** Use CompactVector for ElemsContainer in DMM (and VectorPairDual by
     * extension) */
    static constexpr u32 CV_ELEMS         = 1u << 5;
    /** Use 64 bit types for graph locations and kmers.
     * 32bit is good up to 15 trie depth and 4b graph size
     * 64bit is good up to 31 trie depth and any graph size (hope you have RAM)
     */
    static constexpr u32 WEB_SCALE        = 1u << 6;
};

template<u64 trie_depth = 15,
    u32 flags = CfgFlags::TD_SORTED_VECTOR | CfgFlags::VP_DUAL_IMPL>
struct DnaConfig {
    using Letter = DnaLetter;
    using Letters = DnaLetters;
    using StrHolder = u32;
    using NodeLoc = u32;
    using NodeLen = u32;
    using EdgeLoc = u32;
    using LetterLoc = std::conditional_t<flags & CfgFlags::WEB_SCALE, u64, u32>;
    using KmerHolder = std::conditional_t<flags & CfgFlags::WEB_SCALE, u64, u32>;
    static constexpr bool triedata_allow_inner = flags & CfgFlags::ALLOW_INNER_KMER;
    static constexpr VectorPairsImpl vector_pairs_impl = flags & CfgFlags::VP_DUAL_IMPL ?
        VectorPairsImpl::DUAL : VectorPairsImpl::SIMPLE;
    static constexpr bool trie_pairs_raw = flags & CfgFlags::VP_RAW_KMERS;
    static constexpr u32 TDMapType = 1u; /* use DMM */
    static constexpr bool triedata_sorted_vector = flags & CfgFlags::TD_SORTED_VECTOR;
    static constexpr bool triedata_zero_overhead = flags & CfgFlags::TD_ZERO_OVERHEAD;
    static constexpr bool compactvector_for_elems = flags & CfgFlags::CV_ELEMS;
    static constexpr int LetterLocIdxShift = 4;
    static constexpr u64 KmerLen = trie_depth;
    static constexpr KmerHolder on_mask = KmerHolder(1) << (
            sizeof(KmerHolder) * BITS_PER_BYTE - 1);
};

} /* namespace triedata::dna */

#endif /* __DNA_CONFIG_H__ */
