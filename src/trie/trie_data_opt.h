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

template <typename Kmer_, typename LetterLocData_>
struct TrieDataBaseSMM {
    using Kmer = Kmer_;
    using KHolder = Kmer::Holder;
    using LetterLocData = LetterLocData_;
    using LetterLoc = LetterLocData::LetterLoc;

    SimpleMultimap<KHolder, LetterLoc> trie2graph;
    SimpleMultimap<LetterLoc, KHolder> graph2trie;

    TrieDataBaseSMM() {}

    TrieDataBaseSMM(const TrieDataBaseSMM &) = delete;
    TrieDataBaseSMM &operator= (const TrieDataBaseSMM &) = delete;
    TrieDataBaseSMM(TrieDataBaseSMM &&) = default;
    TrieDataBaseSMM &operator= (TrieDataBaseSMM &&) = default;
};

template <typename Kmer_, typename LetterLocData_,
         typename NumKmers_ = LetterLocData_::LetterLoc>
struct TrieDataBaseDMM {
    using Kmer = Kmer_;
    using KHolder = Kmer::Holder;
    using LetterLocData = LetterLocData_;
    using LetterLoc = LetterLocData::LetterLoc;
    using NumKmers = NumKmers_;

    DenseMultimap<KHolder, NumKmers, LetterLoc/*, SortedVector<NumKmers, u8> */> trie2graph;
    DenseMultimap<LetterLoc, NumKmers, KHolder/*, SortedVector<NumKmers, u8> */> graph2trie;

    TrieDataBaseDMM() {}

    TrieDataBaseDMM(const TrieDataBaseDMM &) = delete;
    TrieDataBaseDMM &operator= (const TrieDataBaseDMM &) = delete;
    TrieDataBaseDMM(TrieDataBaseDMM &&) = default;
    TrieDataBaseDMM &operator= (TrieDataBaseDMM &&) = default;
};

template <typename Kmer_, typename LetterLocData_,
         typename NumKmers_ = LetterLocData_::LetterLoc,
         bool allow_inner = false,
         typename Base =
            /*TrieDataBaseSMM<Kmer_, LetterLocData_>*/
            TrieDataBaseDMM<Kmer_, LetterLocData_, NumKmers_>>
struct TrieDataOpt final : public Base {
    using Kmer = Kmer_;
    using KHolder = Kmer::Holder;
    using LetterLocData = LetterLocData_;
    using LetterLoc = LetterLocData::LetterLoc;
    using NumKmers = NumKmers_;

    using KmerCodec = CodecKmer<Kmer, typename Kmer::Holder, allow_inner>;

    TrieDataOpt(std::vector<std::pair<Kmer, LetterLoc>> pairs,
            const LetterLocData &letter_loc) {
        auto &trie2graph = this->trie2graph;
        auto &graph2trie = this->graph2trie;
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
            typename decltype(Base::trie2graph)::const_key_iterator,
            typename decltype(Base::trie2graph)::const_key_iterator,
            KmerCodec>;
        active_trie = { key_iter_pair(trie2graph.keys()) };
    }

    TrieDataOpt(const TrieDataOpt &) = delete;
    TrieDataOpt &operator= (const TrieDataOpt &) = delete;
    TrieDataOpt(TrieDataOpt &&) = default;
    TrieDataOpt &operator= (TrieDataOpt &&) = default;

//     DenseMultimap<KHolder, NumKmers, LetterLoc/*, SortedVector<NumKmers, u8> */> trie2graph;
//     DenseMultimap<LetterLoc, NumKmers, KHolder/*, SortedVector<NumKmers, u8> */> graph2trie;
    TieredBitset<Kmer, allow_inner> active_trie;

    using t2g_values_view = iter_pair<
        typename decltype(Base::trie2graph)::const_value_iterator,
        typename decltype(Base::trie2graph)::const_value_iterator>;
    t2g_values_view t2g_values_for(Kmer kmer) const {
        return this->trie2graph.values_for(KmerCodec::to_int(kmer));
    }
    bool t2g_contains(Kmer kmer) const {
        return this->trie2graph.contains(KmerCodec::to_int(kmer));
    }

    using g2t_values_view = iter_pair<
        typename decltype(Base::graph2trie)::const_value_iterator,
        typename decltype(Base::graph2trie)::const_value_iterator,
        KmerCodec>;
    g2t_values_view g2t_values_for(LetterLoc loc) const {
        return this->graph2trie.values_for(loc);
    }
    bool g2t_contains(LetterLoc loc) const {
        return this->graph2trie.contains(loc);
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
        return PowHistogram(this->trie2graph.keys() | std::ranges::views::transform([&](const auto &k) {
                    return std::ranges::distance(this->trie2graph.values_for(k));
                }));
    }
    PowHistogram g2t_histogram() const {
        return PowHistogram(this->graph2trie.keys() | std::ranges::views::transform([&](const auto &k) {
                    return std::ranges::distance(this->graph2trie.values_for(k));
                }));
    }
};

} /* namespace triegraph */

#endif /* __TRIE_DATA_OPT_H__ */
