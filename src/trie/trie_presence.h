// SPDX-License-Identifier: MPL-2.0
/*
 * Copyright (c) 2021, Iskren Chernev
 */

#ifndef __TRIE_PRESENCE_H__
#define __TRIE_PRESENCE_H__

#include "util/logger.h"
#include "util/util.h"

namespace triegraph {

template <typename Kmer_, bool allow_inner = false>
struct TriePresence {
    using Kmer = Kmer_;
    using Letter = Kmer::Letter;
    using KHolder = Kmer::Holder;
    static constexpr u64 K = Kmer::K;
    static constexpr u64 num_options = Kmer::Letter::num_options;
    std::vector<bool> present;

    TriePresence() {}
    TriePresence(std::ranges::input_range auto&& c)
        : present(Kmer::NUM_COMPRESSED - Kmer::NUM_LEAFS)
    {
        auto &log = Logger::get();

        log.begin("bitset leafs");
        for (auto kmer : c) {
            // auto kh = KHolder(kmer);
            if constexpr (!allow_inner) {
                // put 1 on last row
                kmer.pop();
                present[kmer.compress()] = 1;
            } else {
                auto lvl = kmer.size();
                if (lvl < Kmer::K) {
                    present[kmer.compress()] = 1;
                } else {
                    kmer.pop();
                    present[kmer.compress()] = 1;
                }
            }
        }
        log.end();
        auto beg_it = Kmer::beg.rend() - Kmer::K;

        // star
        // ++ beg_it;
        // std::cerr << "last level has " << (typename Kmer::TrieElems<Kmer::K-1>)::power << std::endl;
        log.begin("pre-leaves");
        for (u64 pos = present.size() - pow(Letter::num_options, Kmer::K-1) - 1;
                pos < present.size(); --pos) {
            // std::cerr << "looking at " << Kmer::from_compressed(pos) << std::endl;
            if (pos < *beg_it) ++ beg_it;
            if constexpr (allow_inner)
                if (present[pos])
                    continue;
            auto in_lvl = pos - *beg_it;
            for (u64 opt = 0; opt < num_options; ++opt) {
                auto npos = *(beg_it - 1) + ((in_lvl << Letter::bits) | opt);
                if (present[npos]) {
                    // std::cerr << "marking " << Kmer::from_compressed(pos)
                    //     << " due to " << Kmer::from_compressed(npos) << std::endl;
                    present[pos] = 1;
                    break;
                }
            }
        }
        log.end();
    }

    TriePresence(const TriePresence &) = delete;
    TriePresence(TriePresence &&) = default;
    TriePresence &operator= (const TriePresence &) = default;
    TriePresence &operator= (TriePresence &&) = default;

    bool contains(Kmer kmer) const {
        return present[kmer.compress()];
    }

};



} /* namespace triegraph */

#endif /* __TRIE_PRESENCE_H__ */
