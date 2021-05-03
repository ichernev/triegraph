#ifndef __DKMER_H__
#define __DKMER_H__

#include "util/util.h"
#include "trie/kmer_settings.h"

#include <type_traits> /* for is_unsigned_v */
#include <functional>  /* for hash */
#include <iostream>
#include <iterator>
#include <sstream>
#include <type_traits>
#include <algorithm>
#include <span>

namespace triegraph {

// The following crap is used just to compute where each trie-level begins
// at compile time
// template<u64 D, u64 S, template<u64,u64> class F, u64... args>
// struct TrieLevelBeg {
//     static constexpr auto val = TrieLevelBeg<D-1, S, F, F<D,S>::value, args...>::val;
// };
// template<u64 S, template<u64,u64> class F, u64... args>
// struct TrieLevelBeg<0, S, F, args...> {
//     static constexpr std::array<u64, 1 + sizeof...(args)> val = {F<0,S>::value, args...};
// };
// template<u64 depth, u64 split> struct TrieElems {
//     using prev = TrieElems<depth-1, split>;
//     static constexpr u64 power = prev::power * split;
//     static constexpr u64 value = prev::value + prev::power;
// };
// template<u64 split> struct TrieElems<0, split> {
//     static constexpr u64 power = 1;
//     static constexpr u64 value = 0;
// };

template<typename Letter_, typename Holder_>
struct DKmer {
    using Letter = Letter_;
    using Holder = Holder_;
    using klen_type = unsigned short;
    using value_type = Letter;
    using Self = DKmer;

    static constexpr klen_type MAX_K = (sizeof(Holder) * BITS_PER_BYTE - 1) / Letter::bits;
    static inline klen_type K;
    static inline Holder ON_MASK;
    static inline Holder KMER_MASK, EMPTY;
    static inline Holder L1_MASK, L2_MASK, L2_SHIFT, L3_MASK, L3_SHIFT;
    static inline Holder NUM_COMPRESSED, NUM_LEAFS;
    static inline std::array<Holder, sizeof(u64) * BITS_PER_BYTE + 2> beg, lvl_size;

    static Holder _kmer_mask(klen_type anti_len) {
        return (Holder(1) << (K - anti_len) * Letter::bits) - 1;
    }
    static void set_settings(KmerSettings settings) {
        // std::cerr << "setting trie_depth " << settings.trie_depth << '\n'
        //     << "on_mask " << std::hex << settings.on_mask << std::dec << std::endl;
        Self::K = settings.trie_depth;
        Self::ON_MASK = settings.on_mask;
        // std::cerr << "SETTING " << Self::K << " " << std::hex << Self::ON_MASK << std::dec << std::endl;
        Holder h1 = 1;
        klen_type max_k = (sizeof(Holder) * BITS_PER_BYTE - 1) / Letter::bits -
            // make space for on_mask if bits is 1 (num_options == 2)
            (settings.on_mask && Letter::bits == 1 ? 1 : 0);
        if (K > max_k) throw "K is too high";
        Self::KMER_MASK = _kmer_mask(0);
        Self::L1_MASK = h1 << max_k * Letter::bits;
        Self::L2_MASK = Letter::mask;
        Self::L2_SHIFT = (max_k - 1) * Letter::bits;
        Self::L3_MASK = (h1 << 3 * Letter::bits) - 1;
        Self::L3_SHIFT = (max_k - 4) * Letter::bits;
        Self::EMPTY = K > Letter::mask + 1 ?
            ON_MASK | L1_MASK | L2_MASK << L2_SHIFT | Holder(K-Letter::mask-1) << L3_SHIFT :
            ON_MASK | L1_MASK | Holder(K-1) << L2_SHIFT;
        Self::beg[0] = 0;
        Self::lvl_size[0] = 1;
        for (klen_type l = 1; l <= K+1; ++l) {
            Self::beg[l] = Self::beg[l-1] + Self::lvl_size[l-1];
            Self::lvl_size[l] = Self::lvl_size[l-1] * Letter::num_options;
        }
        Self::NUM_COMPRESSED = beg[K+1];
        Self::NUM_LEAFS = lvl_size[K];
    }

    static KmerSettings settings() {
        return { .trie_depth = Self::K, .on_mask = Self::ON_MASK };
    }

    Holder data;
    // receive just the "clean" data, without the len
    // operator Holder() const { return data & _kmer_mask(K - get_len()); }

    // Self() {}
    // explicit Self(Holder data) { this->data = ON_MASK | (data & KMER_MASK); }
    // explicit Self(Holder data, u64 len) {
    //     this->data = ON_MASK | data & _kmer_mask(K-len);
    //     _set_len(len);
    // }
    // Self(const Self &kmer) : data(kmer.data) {}
    // Self(Self &&kmer) : data(kmer.data) {}
    // Self &operator= (const Self &kmer) { data = kmer.data; }
    // Self &operator= (Self &&kmer) { data = kmer.data; }

    static Self from_compressed_leaf(Holder h) { return { ON_MASK | (h & KMER_MASK) }; }
    static Self from_compressed(Holder h) {
        // std::ranges::copy(std::span(beg.begin(), 4), std::ostream_iterator<u32>(std::cerr, " ")); std::cerr << std::endl;
        auto len = std::upper_bound(beg.begin(), beg.begin() + K + 1, h) - beg.begin() - 1;
        // std::cerr << h << " len " << len << std::endl;
        Self k;
        k.data = ON_MASK | ((h-beg[len]) & _kmer_mask(K-len));
        // std::cerr << std::hex << k.data << " ";
        k._set_len(len);
        // std::cerr << std::hex << k.data << std::dec << " " << k.size() << std::endl;
        return k;
    }
    Holder compress_leaf() const { return data & KMER_MASK; }
    Holder compress() const {
        auto sz = size();
        return beg[sz] + (data & _kmer_mask(K-sz));
    }

    static Self empty() { return { EMPTY }; }
    static Self from_str(std::basic_string<typename Letter::Human> s) {
        auto kmer = Self::empty();
        std::istringstream(s) >> kmer;
        return kmer;
    }
    template <typename Cont>
    static Self from_sv(const Cont &c)
        requires std::is_same_v<typename Cont::value_type, Letter>
    {
        auto kmer = Self::empty();
        std::copy(c.begin(), c.end(), std::back_inserter(kmer));
        return kmer;
    }
    std::basic_string<typename Letter::Human> to_str() const {
        std::ostringstream os;
        os << *this;
        return os.str();
    }

    bool is_complete() const {
        return (data & L1_MASK) == 0;
    }

    klen_type get_len() const {
        if (is_complete())
            return K;

        Holder l2 = data >> L2_SHIFT & L2_MASK;
        if (l2 == L2_MASK) {
            Holder l3 = data >> L3_SHIFT & L3_MASK;
            return K - (1 + L2_MASK + l3);
        } else {
            return K - (1 + l2);
        }
    }

    klen_type size() const {
        return get_len();
    }

    void push(Letter l) {
        if (is_complete()) {
            data <<= Letter::bits;
            data |= l.data /* & Letter::bits */;
            data &= KMER_MASK;
            data |= ON_MASK;
        } else {
            Holder mask = _inc_len();
            data = (data & ~mask) | ((data << Letter::bits | l.data) & mask);
        }
    }

    void push_back(Letter l) {
        return push(l);
    }

    void pop() {
        klen_type len = get_len();
        Holder mask = _kmer_mask(K - len);
        data = (data & ~mask) | ((data & mask) >> Letter::bits);
        _dec_len(len);
    }

    void pop_back() {
        pop();
    }

    Letter operator[] (klen_type idx) const {
        return (data >> (size() - idx - 1) * Letter::bits) & Letter::mask;
    }

    Holder _l2_get() const { return data >> L2_SHIFT & L2_MASK; }
    Holder _l3_get() const { return data >> L3_SHIFT & L3_MASK; }
    void _l2_set(Holder l2) {
        data &= ~(L2_MASK << L2_SHIFT);
        data |= l2 << L2_SHIFT;
    }
    void _l3_set(Holder l3) {
        data &= ~(L3_MASK << L3_SHIFT);
        data |= l3 << L3_SHIFT;
    }

    // returns the mask to contain the old
    Holder _inc_len() {
        Holder l2 = _l2_get();
        if (l2 == L2_MASK) {
            Holder l3 = _l3_get();
            if (l3 == 0) {
                goto decr_l2;
            } else {
                _l3_set(--l3);
                return _kmer_mask(1 + l2 + l3);
            }
        } else {
            if (l2 == 0) {
                data ^= L1_MASK;
                return KMER_MASK;
            }
decr_l2:
            _l2_set(--l2);
            return _kmer_mask(1 + l2);
        }
    }

    void _dec_len(klen_type len) {
        klen_type alen = K - len;
        if (alen == 0) {
            data |= L1_MASK;
            // _l2_set(0);
        } else if (alen <= Letter::mask) {
            _l2_set(alen);
        } else {
            _l3_set(alen - Letter::mask);
        }
    }

    void _set_len(klen_type len) {
        klen_type alen = K - len;
        if (alen == 0) {
            // std::cerr << "h1----" << std::endl;
            // std::cerr << std::hex << ON_MASK << std::dec << std::endl;
            // std::cerr << std::hex << L1_MASK << std::dec << std::endl;
            // std::cerr << std::hex << MAX_K << std::dec << std::endl;
            // data |= L1_MASK;
            // _l2_set(0);
        } else if (alen <= Letter::mask) {
            // std::cerr << "h2" << std::endl;
            data |= L1_MASK;
            _l2_set(alen - 1);
        } else {
            // std::cerr << "h3" << std::endl;
            data |= L1_MASK;
            _l2_set(Letter::mask);
            _l3_set(alen - Letter::mask - 1);
        }
    }

    struct ConstKmerIter {
        using iterator_category = std::random_access_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = Letter;
        // using pointer           = value_type*;  // or also value_type*
        using reference         = Letter;
        using Self              = ConstKmerIter;

        reference operator*() const {
            return kmer >> (pos * Letter::bits) & Letter::mask;
        }
        reference operator[](difference_type i) const {
            return kmer >> ((pos-i) * Letter::bits) & Letter::mask;
        }
        Self& operator++() { --pos; return *this; }
        Self operator++(int) { Self tmp = *this; ++(*this); return tmp; }
        Self& operator--() { ++pos; return *this; }
        Self operator--(int) { Self tmp = *this; --(*this); return tmp; }
        Self &operator+=(difference_type i) { pos -= i; return *this; }
        Self operator+ (difference_type i) const { Self tmp = *this; tmp += i; return tmp; }
        friend Self operator+ (difference_type i, const Self &a) { Self tmp = a; tmp += i; return tmp; }
        Self &operator-=(difference_type i) { pos += i; return *this; }
        Self operator- (difference_type i) const { Self tmp = *this; tmp -= i; return tmp; }
        difference_type operator- (const Self &b) const { return b.pos - this->pos; }

        friend bool operator== (const Self& a, const Self& b) { return a.pos == b.pos; }
        friend auto operator<=>(const Self &lhs, const Self &rhs) { return -lhs.pos <=> -rhs.pos; }

        Holder kmer;
        int pos;
    };
    using const_iterator = ConstKmerIter;

    const_iterator begin() const { return const_iterator { data, get_len() - 1 }; }
    const_iterator end() const { return const_iterator { data, -1 }; }

    friend std::ostream &operator<<(std::ostream &os, const Self &kmer) {
        if (os.flags() & std::ios::hex) {
            struct binary_unmapper {
                std::string operator()(Letter l) {
                    return {l.data & 2 ? '1' : '0', l.data & 1 ? '1' : '0'};
                }
            };
            std::transform(kmer.begin(), kmer.end(),
                    std::ostream_iterator<std::string>(os),
                    binary_unmapper());
        } else {
            std::transform(kmer.begin(), kmer.end(),
                    std::ostream_iterator<typename Letter::Human>(os),
                    Letter::Codec::to_ext);
        }
        return os;
    }

    friend std::istream &operator>>(std::istream &is, Self &kmer) {
        using is_it = std::istream_iterator<typename Letter::Human>;

        kmer.data = EMPTY;
        std::transform(is_it(is), is_it(),
                std::back_inserter<Self>(kmer),
                Letter::Codec::to_int);
        return is;
    }

    bool operator== (const Self &other) const { return data == other.data; }
    bool operator== (u64 other) const { return is_complete() && (data & KMER_MASK) == other; }
    bool operator!= (u64 other) const { return !(*this == other); }
    std::strong_ordering operator<=> (const Self &other) const {
        if (auto cmp = size() <=> other.size(); cmp != 0)
            return cmp;
        return (data & _kmer_mask(K-size())) <=> (other.data & _kmer_mask(K-size()));
    }

    // friend struct std::hash<Self> {
    //     static constexpr hash<Holder> hasher();
    //     std::size_t operator() (const Self &kmr) {
    //         return hasher(kmr.data);
    //     }
    // };
    // struct Hash {
    //     static constexpr std::hash<Holder> hasher {};
    //     std::size_t operator() (const Self &kmr) const {
    //         return hasher(kmr.data);
    //     }
    // };
};

} /* namespace triegraph */

template<typename Letter, typename Holder>
struct std::hash<triegraph::DKmer<Letter, Holder>> {
    static constexpr hash<Holder> hasher {};
    std::size_t operator() (const triegraph::DKmer<Letter, Holder> &kmr) const {
        return hasher(kmr.data);
    }
};

#endif /* __DKMER_H__ */

