#ifndef __TRIE_DATA_H__
#define __TRIE_DATA_H__

#include "trie/kmer_codec.h"
#include "trie/trie_presence.h"
#include "util/logger.h"
#include "util/pow_histogram.h"
#include "util/multimaps.h"
#include "util/util.h"
#include "util/vector_pairs.h"

#include <type_traits>
#include <utility>
#include <algorithm>
#include <vector>
#include <array>
#include <iterator>
#include <ranges>
#include <functional>
#include <span>

#include <assert.h>

namespace triegraph {

template <typename Kmer_,
         typename LetterLocData_,
         typename VectorPairs_,
         bool allow_inner,
         typename T2GMap,
         typename G2TMap,
         bool no_overhead_build = false>
struct TrieData {
    using Kmer = Kmer_;
    using KHolder = Kmer::Holder;
    using LetterLocData = LetterLocData_;
    using LetterLoc = LetterLocData::LetterLoc;
    using VectorPairs = VectorPairs_;

    using KmerCodec = triegraph::KmerCodec<Kmer, typename Kmer::Holder, allow_inner>;

    TrieData(VectorPairs pairs, const LetterLocData &letter_loc) {
        if constexpr (no_overhead_build)
            init_dual(std::move(pairs), letter_loc);
        else
            init_single(std::move(pairs), letter_loc);
    }

    void init_single(VectorPairs pairs, const LetterLocData &letter_loc) {
        auto &log = Logger::get();

        auto scope = log.begin_scoped("TrieData init_single");

        log.begin("sort pairs t2g");
        pairs.sort_by_fwd();
        // std::ranges::sort(pairs);
        log.end(); log.begin("build t2g");
        trie2graph = {
                pairs.fwd_pairs()/* | std::ranges::views::transform(
                    [](const auto &p) {
                        return std::make_pair(
                                KmerCodec::to_int(p.first),
                                p.second);
                    }
                )*/};

        log.end(); log.begin("sort pairs g2t");
        pairs.sort_by_rev();
        // std::ranges::sort(pairs, PairSwitchComp<Kmer, LetterLoc> {});
        log.end(); log.begin("build g2t");
        graph2trie = {
                pairs.rev_pairs() /* | std::ranges::views::transform(
                    [](const auto &p) {
                        return std::make_pair(
                                p.second,
                                KmerCodec::to_int(p.first));
                    }
                )*/};

        log.end();
        log.begin("build inner");
        using key_iter_pair = iter_pair<
            typename T2GMap::const_key_iterator,
            typename T2GMap::const_key_iterator,
            KmerCodec>;
        active_trie = { key_iter_pair(trie2graph.keys()) };
    }

    void init_dual(VectorPairs pairs, const LetterLocData &letter_loc) {
        // ideally we should also check if Multimaps use SortedVector and not
        // regular vector
        static_assert(VectorPairs::impl == VectorPairsImpl::DUAL);
        static_assert(T2GMap::impl == MultimapImpl::DENSE);
        static_assert(G2TMap::impl == MultimapImpl::DENSE);
        static_assert(sizeof(KHolder) == sizeof(LetterLoc));

        auto &log = Logger::get();

        auto scope = log.begin_scoped("TrieData init_dual");

        log.begin("sort by rev + unique O(nlogn)");
        pairs.sort_by_rev().unique();
        log.end();

        log.begin("deconstruct pairs O(1)");
        auto kmers = pairs.take_v1();
        auto locs = pairs.take_v2();
        log.end();

        log.begin("build locs_beg O(n)");
        auto locs_beg = G2TMap::StartsContainer::from_elem_seq(locs);
        log.end();

        log.begin("prep kmer_idx O(n)");
        // reuse locs space for kmer_idx
        auto kmer_idx = std::span(locs);
        std::iota(kmer_idx.begin(), kmer_idx.end(), KHolder {});

        log.end().begin("sort kmer_idx O(nlogn)");
        auto kmer_cmp = [&kmers](KHolder a, KHolder b) {
            return kmers[a] < kmers[b];
        };
        std::ranges::sort(kmer_idx, kmer_cmp);

        log.end().begin("prep kmer_beg O(n)");
        auto kmer_beg = T2GMap::StartsContainer::from_elem_seq(
                kmer_idx | std::ranges::views::transform(
                    [&kmers](KHolder a) { return kmers[a]; }));

        log.end().begin("convert kmer_idx -> locs O(nlogn)");
        for (KHolder i = 0; i < kmer_idx.size(); ++i) {
            // auto loc = locs_beg.binary_search(kmer_idx[i]);
            auto loc2 = _bsrch(locs_beg, kmer_idx[i]);
            // if (loc != loc2 || LetterLoc(loc) != LetterLoc(loc2)) {
            //     std::cerr << "i = " << i << " "
            //         << "kidx = " << kmer_idx[i] << " "
            //         << loc << " " << loc2 << std::endl;
            //     assert(0);
            // }
            // NOTE: kmer_idx and locs point to the same memory
            locs[i] = LetterLoc(loc2);
        }

        log.end().begin("construct multi-maps O(1)");
        trie2graph = { std::move(kmer_beg), std::move(locs) };
        graph2trie = { std::move(locs_beg), std::move(kmers) };

        log.end().begin("construct active-trie O(n)");
        using key_iter_pair = iter_pair<
            typename T2GMap::const_key_iterator,
            typename T2GMap::const_key_iterator,
            KmerCodec>;
        active_trie = { key_iter_pair(trie2graph.keys()) };
    }

    auto _bsrch(const auto &arr, auto elem) {
        size_t beg = 0;
        size_t end = arr.size();
        while (beg + 1 < end) {
            size_t mid = (beg + end) / 2;
            if (arr[mid] <= elem)
                beg = mid;
            else
                end = mid;
        }
        return beg;
    }

    TrieData(const TrieData &) = delete;
    TrieData &operator= (const TrieData &) = delete;
    TrieData(TrieData &&) = default;
    TrieData &operator= (TrieData &&) = default;

    T2GMap trie2graph;
    G2TMap graph2trie;
    TriePresence<Kmer, allow_inner> active_trie;

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

        Stats(const Stats &) = delete;
        Stats(Stats &&) = default;
        Stats& operator= (const Stats &) = delete;
        Stats& operator= (Stats &&) = default;

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

#endif /* __TRIE_DATA_H__ */
