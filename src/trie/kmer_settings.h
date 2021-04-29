#ifndef __TRIE_KMER_SETTINGS_H__
#define __TRIE_KMER_SETTINGS_H__

#include "util/util.h"

namespace triegraph {

/// Used for Kmer and DKmer
struct KmerSettings {
    u16 trie_depth;
    u64 on_mask = 0;

    template <typename KmerHolder>
    static KmerSettings from_depth(u16 trie_depth) {
        return {
            .trie_depth = trie_depth,
            .on_mask = default_on_mask<KmerHolder>(),
        };
    }

    template <typename KmerHolder = u32>
    static KmerSettings from_seed_config(
            u64 num_locations,
            const auto &cfg) {
        u32 trie_depth = cfg.template get_or<u32>("trie-depth", 0u);
        i32 trie_depth_rel = cfg.template get_or<i32>("trie-depth-rel", 0);
        if (num_locations == 0ull && trie_depth_rel != 0) {
            throw "trie-depth-rel needs size of graph to work";
        }
        if (trie_depth != 0 && trie_depth_rel != 0) {
            throw "trie-depth and trie-depth-rel are exclusive";
        }
        return {
            .trie_depth = u16(trie_depth ? trie_depth :
                log4_ceil(num_locations) + trie_depth_rel),
            .on_mask = cfg.template get_or<u64>("trie-kmer-on-mask",
                    default_on_mask<KmerHolder>()),
        };
    }

    template <typename KmerHolder>
    static constexpr u64 default_on_mask() {
        return 1ull << (sizeof(KmerHolder) * BITS_PER_BYTE - 1);
    }
};

} /* namespace triegraph */

#endif /* __TRIE_KMER_SETTINGS_H__ */
