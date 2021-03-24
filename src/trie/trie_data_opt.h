#ifndef __TRIE_DATA_OPT_H__
#define __TRIE_DATA_OPT_H__

#include "util/util.h"

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

template <typename A_, typename B_>
struct PairFwd {
    using A = A_;
    using B = B_;
    using pair = std::pair<A, B>;
    struct comparator {
        bool operator() (const std::pair<A, B> &a, const std::pair<A, B> &b) const {
            return a.first == b.first ? a.second < b.second : a.first < b.first;
        }
    };
    static A getA(const std::pair<A, B> &a) { return a.first; }
    static B getB(const std::pair<A, B> &a) { return a.second; }
};

template <typename A_, typename B_>
struct PairRev {
    using A = A_;
    using B = B_;
    using pair = std::pair<A, B>;
    struct comparator {
        bool operator() (const std::pair<A, B> &a, const std::pair<A, B> &b) const {
            return a.second == b.second ? a.first < b.first : a.second < b.second;
        }
    };
    static B getA(const std::pair<A, B> &a) { return a.second; }
    static A getB(const std::pair<A, B> &a) { return a.first; }
};

template <typename Kmer>
struct CookKmer {
    typename Kmer::Holder operator() (const Kmer &k) const {
        return k.compress_leaf();
    }
};

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

template <typename Kmer, typename KmerComp = Kmer::Holder>
struct CodecKmerLeaf {
    using ext_type = Kmer;
    using int_type = KmerComp;

    static int_type to_int(const ext_type &kmer) { return kmer.compress_leaf(); }
    static ext_type to_ext(const int_type &krepr) { return Kmer::from_compressed_leaf(krepr); }
};

template <typename A, typename B, typename C>
struct OptMMap {
    template<typename Accessor, typename CookA = std::identity, typename CookB = std::identity>
    void init(std::vector<typename Accessor::pair> &pairs, u64 a_max) {
        CookA cookA;
        CookB cookB;
        using comp = Accessor::comparator;
        // std::cerr << "sorting" << std::endl;
        std::sort(pairs.begin(), pairs.end(), comp());

        // std::cerr << "reserving" << std::endl;
        start.reserve(a_max);
        elems.reserve(pairs.size());
        // end.resize(pairs.size(), false);
        // std::cerr << "pushing..." << std::endl;
        auto cp = pairs.begin(), ep = pairs.end();
        for (u64 a = 0; a < a_max; ++a) {
            // std::cerr << "now at a " << a << std::endl;
            // std::cerr << Accessor::getA(*cp) << " "
            //     << A(Accessor::getA(*cp)) << std::endl;
            start.push_back(elems.size());
            if (cp == ep || cookA(Accessor::getA(*cp)) != a) {
                continue;
            }
            // end[elems.size()] = true;
            while (cp != ep && Accessor::getA(*cp) == a) {
                elems.push_back(cookB(Accessor::getB(*cp)));
                ++cp;
            }
        }
    }

    size_t size() const { return elems.size(); }
    // size_t key_size() const { return elems.size(); }

    struct PairIter {
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = std::pair<A, C>;
        using reference_type = value_type;

        using Self = PairIter;
        using Parent = OptMMap;

        PairIter() : p(nullptr), a(), b() {}
        PairIter(const Parent &p) : p(&p), a(p.start.size()), b(p.elems.size()) {}
        PairIter(const Parent &p, A a)
            : p(&p), a(a), b(a < p.start.size() ? p.start[a] : p.elems.size())
        {
            adjust();
        }
        reference_type operator* () const {
            // std::cerr << "IN **" << a << " " << b << " " << p->elems[b] << std::endl;
            return {a, p->elems[b]};
        }
        Self &operator++ () {
            // std::cerr << "a = " << a << " " << p->bound(a) << std::endl;
            if (++b == p->bound(a))
                adjust();
            return *this;
        }
        Self operator++ (int) { Self tmp = *this; ++(*this); return tmp; }
        bool operator== (const Self &other) const { return b == other.b; }
        difference_type operator- (const Self &other) const { return b - other.b; }

    private:
        void adjust() {
            while (a < p->start.size() && p->bound(a) <= b)
                ++a;
        }

        const Parent *p;
        // TODO: for k==16, the holder might be u32, but it won't have space
        // for the sentinel, so use u64, at least for A
        A a;
        B b;
    };

    struct KeyIter {
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = A;
        using reference_type = value_type;
        using Self = KeyIter;
        using Parent = OptMMap;

        KeyIter() : p(nullptr), a() {}
        KeyIter(const Parent &p) : p(&p), a(p.start.size()) {}
        KeyIter(const Parent &p, A a) : p(&p), a(a) { adjust(); }

        reference_type operator* () const { return a; }
        Self &operator++ () { ++a; adjust(); return *this; }
        Self operator++ (int) { Self tmp = *this; ++(*this); return tmp; }
        bool operator== (const Self &other) const { return a == other.a; }

        void adjust() {
            // skip "empty" keys
            while (a < p->start.size() && p->start[a] == p->bound(a))
                ++a;
        }

        const Parent *p;
        // TODO: for k==16, the holder might be u32, but it won't have space
        // for the sentinel, so use u64, at least for A
        A a;
    };

    struct ValIter {
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = C;
        using reference_type = value_type;
        using Self = ValIter;

        ValIter() : it() {}
        ValIter(std::vector<value_type>::const_iterator it) : it(it) {}

        reference_type operator* () const { return *it; }
        Self &operator++ () { ++it; return *this; }
        Self operator++ (int) { Self tmp = *this; ++(*this); return tmp; }
        difference_type operator- (const Self &other) { return it - other.it; }
        bool operator== (const Self &other) const { return it == other.it; }

        std::vector<value_type>::const_iterator it;
    };

    using const_iterator = PairIter;
    using const_key_iterator = KeyIter;
    using const_value_iterator = ValIter;

    const_iterator begin() const { return const_iterator(*this, 0); }
    const_iterator end() const { return const_iterator(*this); }
    iter_pair<const_key_iterator, const_key_iterator> keys() const {
        return { const_key_iterator(*this, 0), const_key_iterator(*this) };
    }
    iter_pair<const_value_iterator, const_value_iterator> values_for(const A &a) const {
        return { elems.begin() + start[a], elems.begin() + bound(a) };
    }

    std::pair<const_iterator, const_iterator> equal_range(const A &a) const {
        // for k=16 a+1 might overflow
        return std::make_pair(const_iterator(*this, a), const_iterator(*this, a+1));
    }

    bool contains(const A &a) const { return start[a] < bound(a); }

private:
    B bound(A a) const {
        if (a + 1 >= start.size())
            return elems.size();
        return start[a+1];
    }

    std::vector<B> start; // indexed by A
    std::vector<C> elems; // indexed by B
};

template <typename Kmer_, bool allow_inner = false>
struct TieredBitset {
    using Kmer = Kmer_;
    using Letter = Kmer::Letter;
    using KHolder = Kmer::Holder;
    static constexpr u64 K = Kmer::K;
    static constexpr u64 num_options = Kmer::Letter::num_options;
    std::vector<bool> present;

    TieredBitset() : present(Kmer::NUM_COMPRESSED - Kmer::NUM_LEAFS) {}

    // template <std::ranges::range C>
    // void init(C c) { init(c.begin(), c.end()); }

    // template<typename IT_B, typename IT_E>
    //     requires std::is_same_v<typename std::iterator_traits<IT_B>::value_type, Kmer> &&
    //             std::sentinel_for<IT_E, IT_B>
    // void init(IT_B begin, IT_E end) {
    template <std::ranges::range C>
        // requires /* std::ranges::view<C> && */ std::is_same_v<C::value_type, Kmer>
    void init(const C &c) {
        for (auto kmer : c) {
            // auto kh = KHolder(kmer);
            if constexpr (!allow_inner) {
                // put 1 on last row
                kmer.pop();
                present[kmer.compress()] = 1;
            } else {
                assert(false);
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

    bool contains(Kmer kmer) const {
        return present[kmer.compress()];
    }
};

template <typename Kmer_, typename LetterLocData_,
         typename NumKmers_ = LetterLocData_::LetterLoc>
struct TrieDataOpt {
    using Kmer = Kmer_;
    using KHolder = Kmer::Holder;
    using LetterLocData = LetterLocData_;
    using LetterLoc = LetterLocData::LetterLoc;
    using NumKmers = NumKmers_;

    using KmerCodec = CodecKmerLeaf<Kmer>;

    TrieDataOpt(std::vector<std::pair<Kmer, LetterLoc>> pairs,
            const LetterLocData &letter_loc) {
        auto maxkmer = pow(Kmer::Letter::num_options, Kmer::K);
        using Fwd = PairFwd<Kmer, LetterLoc>;
        using Rev = PairRev<Kmer, LetterLoc>;
        // for (auto p : pairs) {
        //     std::cerr << p << std::endl;
        // }
        // std::copy(pairs.begin(), pairs.end(), std::output_iterator<std::pair<Kmer, LetterLoc>>(
        //             std::cerr, "\n"));
        // std::cerr << "init 1" << std::endl;
        trie2graph.template init<Fwd, CookKmer<Kmer>, std::identity>(
                pairs, maxkmer /* , letter_loc.num_locations */);
        // std::cerr << "init 2" << std::endl;
        graph2trie.template init<Rev, std::identity, CookKmer<Kmer>>(
                pairs, letter_loc.num_locations /* , maxkmer */);
        // std::cerr << "init done" << std::endl;
        // TODO: If supporting inner-nodes this should be reworked.
        // std::cerr << "Keys are: " << std::endl;
        // for (auto x : trie2graph.keys()) {
        //     std::cerr << KmerCodec::to_ext(x) << std::endl;
        // }
        using key_iter_pair = iter_pair<
            typename decltype(trie2graph)::const_key_iterator,
            typename decltype(trie2graph)::const_key_iterator,
            KmerCodec>;
        active_trie.init(key_iter_pair(trie2graph.keys()));
                // std::views::transform([](KHolder kh) { return Kmer(kh); }));
    }
    TrieDataOpt(const TrieDataOpt &) = delete;
    TrieDataOpt &operator= (const TrieDataOpt &) = delete;
    TrieDataOpt(TrieDataOpt &&) = default;
    TrieDataOpt &operator= (TrieDataOpt &&) = default;

    OptMMap<KHolder, NumKmers, LetterLoc> trie2graph;
    OptMMap<LetterLoc, NumKmers, KHolder> graph2trie;
    TieredBitset<Kmer> active_trie;

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
};

} /* namespace triegraph */

#endif /* __TRIE_DATA_OPT_H__ */
