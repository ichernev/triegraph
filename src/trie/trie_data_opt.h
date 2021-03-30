#ifndef __TRIE_DATA_OPT_H__
#define __TRIE_DATA_OPT_H__

#include "util/util.h"
#include "util/pow_histogram.h"
#include "util/dense_multimap.h"

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

// template <typename A_, typename B_>
// struct PairFwd {
//     using A = A_;
//     using B = B_;
//     using pair = std::pair<A, B>;
//     struct comparator {
//         bool operator() (const std::pair<A, B> &a, const std::pair<A, B> &b) const {
//             return a.first == b.first ? a.second < b.second : a.first < b.first;
//         }
//     };
//     static A getA(const std::pair<A, B> &a) { return a.first; }
//     static B getB(const std::pair<A, B> &a) { return a.second; }
// };

// template <typename A_, typename B_>
// struct PairRev {
//     using A = A_;
//     using B = B_;
//     using pair = std::pair<A, B>;
//     struct comparator {
//         bool operator() (const std::pair<A, B> &a, const std::pair<A, B> &b) const {
//             return a.second == b.second ? a.first < b.first : a.second < b.second;
//         }
//     };
//     static B getA(const std::pair<A, B> &a) { return a.second; }
//     static A getB(const std::pair<A, B> &a) { return a.first; }
// };

// template <typename Kmer, bool allow_inner = false>
// struct CookKmer {
//     typename Kmer::Holder operator() (const Kmer &k) const {
//         return k.compress_leaf();
//     }
// };
// template <typename Kmer>
// struct CookKmer<Kmer, true> {
//     typename Kmer::Holder operator() (const Kmer &k) const {
//         return k.compress();
//     }
// };

// Codec::ext_type
// Codec::int_type
// Codec::to_int(ext_type)
// Codec::to_ext(int_type)
// template <typename Ext, typename Int = Ext>
// struct CodecIdentity {
//     using ext_type = Ext;
//     using int_type = Int;

//     int_type to_int(const ext_type &ext) const { return ext; }
//     ext_type to_ext(const int_type &in) const { return in; }
// };

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
    TieredBitset(std::ranges::range auto const& c)
        : present(Kmer::NUM_COMPRESSED - Kmer::NUM_LEAFS)
    {
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
        auto beg_it = Kmer::beg.rend() - Kmer::K;
        // star
        // ++ beg_it;
        // std::cerr << "last level has " << (typename Kmer::TrieElems<Kmer::K-1>)::power << std::endl;
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
    }

    TieredBitset(const TieredBitset &) = delete;
    TieredBitset(TieredBitset &&) = default;
    TieredBitset &operator= (const TieredBitset &) = default;
    TieredBitset &operator= (TieredBitset &&) = default;

    bool contains(Kmer kmer) const {
        return present[kmer.compress()];
    }

};

template <typename Kmer_, typename LetterLocData_,
         typename NumKmers_ = LetterLocData_::LetterLoc,
         bool allow_inner = false>
struct TrieDataOpt {
    using Kmer = Kmer_;
    using KHolder = Kmer::Holder;
    using LetterLocData = LetterLocData_;
    using LetterLoc = LetterLocData::LetterLoc;
    using NumKmers = NumKmers_;

    using KmerCodec = CodecKmer<Kmer, typename Kmer::Holder, allow_inner>;

    TrieDataOpt(std::vector<std::pair<Kmer, LetterLoc>> pairs,
            const LetterLocData &letter_loc) {
        // u64 maxkmer = total_kmers();
        // using Fwd = PairFwd<Kmer, LetterLoc>;
        // using Rev = PairRev<Kmer, LetterLoc>;

        // sort by pair::first, then ::second
        std::ranges::sort(pairs);
        trie2graph = {
                pairs | std::ranges::views::transform(
                    [](const auto &p) {
                        return std::make_pair(
                                KmerCodec::to_int(p.first),
                                p.second);
                    }
                )};

        std::ranges::sort(pairs, PairSwitchComp<Kmer, LetterLoc> {});
        graph2trie = {
                pairs | std::ranges::views::transform(
                    [](const auto &p) {
                        return std::make_pair(
                                p.second,
                                KmerCodec::to_int(p.first));
                    }
                )};
        // trie2graph.template init<Fwd, CookKmer<Kmer, allow_inner>, std::identity>(
        //         pairs, maxkmer /* , letter_loc.num_locations */);
        // graph2trie.template init<Rev, std::identity, CookKmer<Kmer, allow_inner>>(
        //         pairs, letter_loc.num_locations /* , maxkmer */);
        using key_iter_pair = iter_pair<
            typename decltype(trie2graph)::const_key_iterator,
            typename decltype(trie2graph)::const_key_iterator,
            KmerCodec>;
        active_trie = { key_iter_pair(trie2graph.keys()) };
    }

    void sanity_check(std::vector<std::pair<Kmer, LetterLoc>> pairs,
            const LetterLocData &letter_loc) {
        // using Fwd = PairFwd<Kmer, LetterLoc>;
        // using Rev = PairRev<Kmer, LetterLoc>;
        u64 maxkmer = total_kmers();
        // test trie2graph
        std::ranges::sort(pairs);
        assert(std::ranges::equal(
                    pairs | std::ranges::views::transform(
                        [](const auto &p) {
                        return std::make_pair(
                                KmerCodec::to_int(p.first),
                                p.second);
                        }),
                    trie2graph));

        auto pbeg = pairs.begin(), pend = pairs.end();
        for (u64 i = 0; i < maxkmer; ++i) {
            if (pbeg != pend && pbeg->first == KmerCodec::to_ext(i)) {
                assert(trie2graph.contains(i));
                auto vv = trie2graph.values_for(i);
                assert(!vv.empty());
                while (!vv.empty()) {
                    assert(*vv == pbeg->second);
                    ++vv;
                    ++pbeg;
                }
            } else {
                // test empty range
                assert(!trie2graph.contains(i));
                assert(trie2graph.values_for(i).empty());
                auto er = trie2graph.equal_range(i);
                assert(er.first == er.second);
            }
        }

        std::ranges::sort(pairs, PairSwitchComp<Kmer, LetterLoc> {});
        assert(std::ranges::equal(
                    pairs | std::ranges::views::transform(
                        [](const auto &p) {
                        return std::make_pair(
                                p.second,
                                KmerCodec::to_int(p.first));
                        }
                        ),
                    graph2trie));
        pbeg = pairs.begin(), pend = pairs.end();
        for (u64 i = 0; i < letter_loc.num_locations; ++i) {
            if (pbeg != pend && pbeg->second == i) {
                assert(graph2trie.contains(i));
                auto vv = graph2trie.values_for(i);
                assert(!vv.empty());
                while (!vv.empty()) {
                    assert(*vv == KmerCodec::to_int(pbeg->first));
                    ++vv;
                    ++pbeg;
                }
            } else {
                // test empty range
                assert(!graph2trie.contains(i));
                assert(graph2trie.values_for(i).empty());
                auto er = graph2trie.equal_range(i);
                assert(er.first == er.second);
            }
        }
    }

    TrieDataOpt(const TrieDataOpt &) = delete;
    TrieDataOpt &operator= (const TrieDataOpt &) = delete;
    TrieDataOpt(TrieDataOpt &&) = default;
    TrieDataOpt &operator= (TrieDataOpt &&) = default;

    DenseMultimap<KHolder, NumKmers, LetterLoc> trie2graph;
    DenseMultimap<LetterLoc, NumKmers, KHolder> graph2trie;
    TieredBitset<Kmer, allow_inner> active_trie;

    using t2g_values_view = iter_pair<
        typename decltype(trie2graph)::const_value_iterator,
        typename decltype(trie2graph)::const_value_iterator>;
    t2g_values_view t2g_values_for(Kmer kmer) const {
        return trie2graph.values_for(KmerCodec::to_int(kmer));
    }
    bool t2g_contains(Kmer kmer) const {
        return trie2graph.contains(KmerCodec::to_int(kmer));
    }

    using g2t_values_view = iter_pair<
        typename decltype(graph2trie)::const_value_iterator,
        typename decltype(graph2trie)::const_value_iterator,
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

    PowHistogram t2g_histogram() const {
        return PowHistogram(trie2graph.keys() | std::ranges::views::transform([&](const auto &k) {
                    return std::ranges::distance(trie2graph.values_for(k));
                }));
    }
    PowHistogram g2t_histogram() const {
        return PowHistogram(graph2trie.keys() | std::ranges::views::transform([&](const auto &k) {
                    return std::ranges::distance(graph2trie.values_for(k));
                }));
    }
};

} /* namespace triegraph */

#endif /* __TRIE_DATA_OPT_H__ */
