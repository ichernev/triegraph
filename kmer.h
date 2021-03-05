#ifndef __KMER_H__
#define __KMER_H__

#include "util.h"
#include <type_traits> /* for is_unsigned_v */
#include <functional>  /* for hash */
#include <iostream>
#include <iterator>

template<typename Letter_, typename Holder_, unsigned k>
struct Kmer {
    using Letter = Letter_;
    using Holder = Holder_;
    using klen_type = unsigned short;
    // use one bit to encode less-than-maxk length
    static constexpr Holder H1 = 1;
    static constexpr klen_type MAX_K = (sizeof(Holder) * BITS_PER_BYTE - 1) / Letter::bits;
    static constexpr klen_type K = k;
    static constexpr Holder _kmer_mask(klen_type anti_len) {
        return (H1 << (K - anti_len) * Letter::bits) - 1;
    }
    static constexpr Holder KMER_MASK = _kmer_mask(0);
    static constexpr Holder L1_MASK = H1 << MAX_K * Letter::bits;
    static constexpr Holder L2_MASK = Holder(Letter::mask);
    static constexpr Holder L2_SHIFT = (MAX_K - 1) * Letter::bits;
    static constexpr Holder L3_MASK = (H1 << 3 * Letter::bits) - 1;
    static constexpr Holder L3_SHIFT = (MAX_K - 4) * Letter::bits;
    static constexpr Holder EMPTY = L1_MASK | L2_MASK << L2_SHIFT | Holder(K-4) << L3_SHIFT;
    static_assert(std::is_unsigned_v<Holder>, "holder should be unsigned");
    static_assert(k <= MAX_K, "can not support big k, increase holder size");

    Holder data;

    static Kmer empty() {
        return Kmer {EMPTY};
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

    void push(Letter l) {
        if (is_complete()) {
            // NOTE: this doesn't preserve "waste" bits
            data <<= Letter::bits;
            data |= l.data /* & Letter::bits */;
            data &= KMER_MASK;
        } else {
            Holder mask = _inc_len();
            data = (data & ~mask) | ((data << Letter::bits | l.data) & mask);
        }
    }

    void pop() {
        klen_type len = get_len();
        Holder mask = _kmer_mask(K - len);
        data = (data & ~mask) | ((data & mask) >> Letter::bits);
        _dec_len(len);
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
        } else if (alen < 4) {
            _l2_set(alen);
        } else {
            _l3_set(alen - 3);
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

    friend std::ostream &operator<<(std::ostream &os, const Kmer &kmer) {
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
            typename Letter::Unmapper unmapper;
            std::transform(kmer.begin(), kmer.end(),
                    std::ostream_iterator<typename Letter::Human>(os),
                    unmapper);
        }
        return os;
    }

    // friend struct std::hash<Kmer> {
    //     static constexpr hash<Holder> hasher();
    //     std::size_t operator() (const Kmer &kmr) {
    //         return hasher(kmr.data);
    //     }
    // };
};

template<typename Letter, typename Holder, unsigned k>
struct std::hash<Kmer<Letter, Holder, k>> {
    static constexpr hash<Holder> hasher();
    std::size_t operator() (const Kmer<Letter, Holder, k> &kmr) {
        return hasher(kmr.data);
    }
};


#endif /* __KMER_H__ */