#ifndef __TRIE_DATA_OPT_H__
#define __TRIE_DATA_OPT_H__

#include "util/util.h"
#include "util/pow_histogram.h"
#include "util/dense_multimap.h"
#include "util/simple_multimap.h"
#include "util/sorted_vector.h"
#include "util/logger.h"

#include <type_traits>
#include <utility>
#include <algorithm>
#include <vector>
#include <array>
#include <iterator>
#include <ranges>
#include <functional>

#include <assert.h>

namespace triegraph {

template <typename A, typename B>
struct PairSwitchComp {
    bool operator() (const std::pair<A, B> &a, const std::pair<A, B> &b) const {
        return a.second == b.second ? a.first < b.first : a.second < b.second;
    }
};

template <typename Kmer, typename KmerComp = Kmer::Holder, bool allow_inner = false>
struct CodecKmer {
    using ext_type = Kmer;
    using int_type = KmerComp;

    static int_type to_int(const ext_type &kmer) { return kmer.compress_leaf(); }
    static ext_type to_ext(const int_type &krepr) { return Kmer::from_compressed_leaf(krepr); }
};
template <typename Kmer, typename KmerComp>
struct CodecKmer<Kmer, KmerComp, true> {
    using ext_type = Kmer;
    using int_type = KmerComp;

    static int_type to_int(const ext_type &kmer) { return kmer.compress(); }
    static ext_type to_ext(const int_type &krepr) { return Kmer::from_compressed(krepr); }
};

template <typename Kmer_, bool allow_inner = false>
struct TieredBitset {
    using Kmer = Kmer_;
    using Letter = Kmer::Letter;
    using KHolder = Kmer::Holder;
    static constexpr u64 K = Kmer::K;
    static constexpr u64 num_options = Kmer::Letter::num_options;
    std::vector<bool> present;

    TieredBitset() {}
    TieredBitset(std::ranges::input_range auto&& c)
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

    TieredBitset(const TieredBitset &) = delete;
    TieredBitset(TieredBitset &&) = default;
    TieredBitset &operator= (const TieredBitset &) = default;
    TieredBitset &operator= (TieredBitset &&) = default;

    bool contains(Kmer kmer) const {
        return present[kmer.compress()];
    }

};

template <typename Kmer_,
         typename LetterLocData_,
         bool allow_inner,
         typename T2GMap,
         typename G2TMap>
struct TrieData {
    using Kmer = Kmer_;
    using KHolder = Kmer::Holder;
    using LetterLocData = LetterLocData_;
    using LetterLoc = LetterLocData::LetterLoc;

    using KmerCodec = CodecKmer<Kmer, typename Kmer::Holder, allow_inner>;

    TrieData(std::vector<std::pair<Kmer, LetterLoc>> pairs,
            const LetterLocData &letter_loc) {
        auto &log = Logger::get();

        auto scope = log.begin_scoped("trie data");

        log.begin("sort pairs t2g");
        std::ranges::sort(pairs);
        log.end(); log.begin("build t2g");
        trie2graph = {
                pairs | std::ranges::views::transform(
                    [](const auto &p) {
                        return std::make_pair(
                                KmerCodec::to_int(p.first),
                                p.second);
                    }
                )};

        log.end(); log.begin("sort pairs g2t");
        std::ranges::sort(pairs, PairSwitchComp<Kmer, LetterLoc> {});
        log.end(); log.begin("build g2t");
        graph2trie = {
                pairs | std::ranges::views::transform(
                    [](const auto &p) {
                        return std::make_pair(
                                p.second,
                                KmerCodec::to_int(p.first));
                    }
                )};

        log.end();
        log.begin("build inner");
        using key_iter_pair = iter_pair<
            typename T2GMap::const_key_iterator,
            typename T2GMap::const_key_iterator,
            KmerCodec>;
        active_trie = { key_iter_pair(trie2graph.keys()) };
    }

    TrieData(const TrieData &) = delete;
    TrieData &operator= (const TrieData &) = delete;
    TrieData(TrieData &&) = default;
    TrieData &operator= (TrieData &&) = default;

    T2GMap trie2graph;
    G2TMap graph2trie;
    TieredBitset<Kmer, allow_inner> active_trie;

    using t2g_values_view = iter_pair<
        typename T2GMap::const_value_iterator,
        typename T2GMap::const_value_iterator>;
    t2g_values_view t2g_values_for(Kmer kmer) const {
        return trie2graph.values_for(KmerCodec::to_int(kmer));
    }
    bool t2g_contains(Kmer kmer) const {
        return trie2graph.contains(KmerCodec::to_int(kmer));
    }

    using g2t_values_view = iter_pair<
        typename G2TMap::const_value_iterator,
        typename G2TMap::const_value_iterator,
        KmerCodec>;
    g2t_values_view g2t_values_for(LetterLoc loc) const {
        return graph2trie.values_for(loc);
    }
    bool g2t_contains(LetterLoc loc) const {
        return graph2trie.contains(loc);
    }

    bool trie_inner_contains(Kmer kmer) const {
        return active_trie.contains(kmer);
    }

    bool trie_contains(Kmer kmer) const {
        if (kmer.is_complete()) {
            return t2g_contains(kmer);
        } else {
            return trie_inner_contains(kmer);
        }
    }

    static constexpr u64 total_kmers() {
        if constexpr (allow_inner) {
            return Kmer::NUM_COMPRESSED;
        } else {
            return Kmer::NUM_LEAFS;
        }
    }

    struct Stats {
        PowHistogram t2g_hist;
        PowHistogram g2t_hist;
        KHolder num_kmers;
        LetterLoc num_locs;
        KHolder num_pairs;

        Stats(const TrieData &td)
            : t2g_hist(td.trie2graph.keys() | std::ranges::views::transform(
                        [&td](const auto &k) {
                            return std::ranges::distance(td.trie2graph.values_for(k));
                        })),
              g2t_hist(td.graph2trie.keys() | std::ranges::views::transform(
                        [&td](const auto &k) {
                            return std::ranges::distance(td.graph2trie.values_for(k));
                        })),
              num_kmers(std::ranges::distance(td.trie2graph.keys())),
              num_locs(std::ranges::distance(td.graph2trie.keys())),
              num_pairs(td.trie2graph.size())
        {}

        friend std::ostream &operator<< (std::ostream &os, const Stats &s) {
            return os
                << "T2G Histogrm:\n" << s.t2g_hist
                << "G2T Histogram:\n" << s.g2t_hist
                << "num kmers: " << s.num_kmers << '\n'
                << "num locs: " << s.num_locs << '\n'
                << "ff: " << double(s.num_kmers) / s.num_locs << '\n'
                << "pairs: " << s.num_pairs;
        }
    };

    Stats stats() const { return Stats(*this); }
};

} /* namespace triegraph */

#endif /* __TRIE_DATA_OPT_H__ */
