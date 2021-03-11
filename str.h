#ifndef __STR_H__
#define __STR_H__

#include "util.h"
#include <string>
#include <algorithm>
#include <iterator>

namespace triegraph {

template <
    typename Letter_,
    typename Holder_ = typename Letter_::Holder,
    typename Size_ = u32>
struct Str {
    using Letter = Letter_;
    using Holder = Holder_;
    using Size = Size_;
    using Input = const std::basic_string<typename Letter::Human> &;
    // using letter_human_type = typename Letter::human_type;
    // static constexpr int letter_bits = Letter::bits;
    // static constexpr int letters_per_holder = sizeof(Letter) * BITS_PER_BYTE / letter_bits;
    static constexpr int letters_per_store  = sizeof(Holder) * BITS_PER_BYTE / Letter::bits;

    Holder *data;
    Size length;   /* number of letters */
    Size capacity; /* number of Holders */

    template <typename ISize_ = u8>
    struct ConstStrIter {
        using ISize = ISize_;
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = Letter;
        // using pointer           = value_type*;  // or also value_type*
        using reference         = Letter;
        using Self              = ConstStrIter;

        static constexpr ISize letters_per_holder =
            sizeof(Holder) * BITS_PER_BYTE / Letter::bits;

        ConstStrIter() {}
        ConstStrIter(const Holder *data_, short cid_) : data(data_), cid(cid_) {
            Size off = cid / letters_per_holder;
            ISize iid = cid % letters_per_holder;

            data += off;
            cid = iid;
        }

        reference operator*() const { return *data >> cid * Letter::bits & Letter::mask; }
        // pointer operator->() const { return &foo; }

        Self& operator++() {
            if (++cid == letters_per_holder) {
                cid = 0;
                ++data;
            }
            return *this;
        }
        Self operator++(int) { Self tmp = *this; ++(*this); return tmp; }

        friend bool operator== (const Self& a, const Self& b) = default;
    private:
        const Holder *data;
        ISize cid;
    };
    using const_iterator = ConstStrIter<u8>;

    struct View {
        const Str *base;
        Size offset;
        Size length;

        View(const Str *base_, Size offset_, Size length_)
            : base(base_), offset(offset_), length(length_) {}

        Letter operator[](Size idx) const { return (*base)[idx + offset]; }
        Letter at(Size idx) const {
            if (idx < length) {
                return base->at(idx);
            }
            throw "out-of-bounds";
        }

        // Holder _store(Size idx) const {
        //     if (idx + offset % letters_per_holder != 0) {
        //         throw "incorrect-store-access";
        //     }
        //     return base->data[(idx+offset) / letters_per_holder];
        // }

        Size fast_match(const View &other) const {
            Size mlen = std::min(length, other.length), i;
            for (i = 0; i < mlen && (*this)[i] == other[i]; ++i);
            return i;
        }

        // Size fast_match_2(const View &other) const {
        //     if ((offset - other.offset) % letters_per_store != 0) {
        //         return fast_match(other);
        //     }
        //     // do the first part slow
        //     int slow_search = (letters_per_store - offset % letters_per_store) % letters_per_store;
        //     Size mlen = std::min(length, other.length), i;
        //     for (i = 0; i < slow_search && i < mlen && (*this)[i] == other[i]; ++i);
        //     if (i < slow_search && i < mlen) {
        //         return i;
        //     }
        //     // store by store
        //     for (i = slow_search; i < mlen && (*this)._store(i) == other._store(i); i += letters_per_store);
        //     // last part slow
        //     for (; i < mlen && (*this)[i] == other[i]; ++i);
        //     return i;
        // }


        friend std::ostream &operator<<(std::ostream &os, const View &sv) {
            typename Letter::Decoder letter_dec;
            std::transform(sv.begin(), sv.end(),
                    std::ostream_iterator<typename Letter::Human>(os),
                    letter_dec);
            return os;
        }

        // friend std::ostream &operator<<(std::ostream &os, const View &sv) {
        //     typename Letter::Unmapper unmapper;

        //     for (Size i = 0; i < sv.length; ++i) {
        //         os << unmapper(sv[i]);
        //     }
        //     return os;
        // }

        const_iterator begin() const { return const_iterator(base->data, offset); }
        const_iterator end() const { return const_iterator(base->data, offset + length); }
    };

    Str() { this->data = nullptr; }
    Str(Input human) : data(nullptr), length(0), capacity(0) { append(human); }
    ~Str() { free(this->data); }

    Str(const Str &) = delete;
    Str &operator=(const Str &) = delete;

    Str(Str &&other) : data(other.data), length(other.length), capacity(other.capacity) {
        other.data = nullptr;
    }
    Str &operator=(Str &&other) {
        std::swap(data, other.data);
        length = other.length;
        capacity = other.capacity;
        other.clear();
        return *this;
    }

    Size size() const {
        return length;
    }

    void clear() {
        free(this->data);
        this->data = nullptr;
        length = 0;
        capacity = 0;
    }

    Size append(Input human) {
        auto letter_enc = typename Letter::Encoder();

        Size ncap = div_up(this->length + human.size(), letters_per_store);
        if (ncap > this->capacity) {
            this->capacity = ncap;
            this->data = static_cast<Holder *>(
                    realloc(this->data, ncap * sizeof(Holder)));
        }

        Holder *oit = this->data + (this->length / letters_per_store);
        Size sub_idx = this->length % letters_per_store;
        if (sub_idx == 0) *oit = 0;
        for (auto iit = human.begin(); iit != human.end(); ++iit, ++this->length) {
            if (sub_idx == letters_per_store) {
                sub_idx = 0;
                oit += 1;
                *oit = 0;
            }
            Letter bin = letter_enc(*iit);
            *oit |= bin << (sub_idx * Letter::bits);
            sub_idx += 1;
        }
        return this->length;
    }

    operator View() const { return get_view(); }

    View get_view(Size offset, Size length) const { return View(this, offset, length); }
    View get_view(Size offset) const { return get_view(offset, length - offset); }
    View get_view() const { return get_view(0, length); }

    Letter operator[](Size idx) const {
        Size cell = idx / letters_per_store;
        int cell_bit = (idx % letters_per_store) * Letter::bits;

        return static_cast<Letter>((this->data[cell] >> cell_bit) & Letter::mask);
    }

    void set(Size idx, Letter val) {
        Size cell = idx / letters_per_store;
        int cell_bit = (idx % letters_per_store) * Letter::bits;
        this->data[cell] &= Letter::mask << cell_bit;
        this->data[cell] |= val << cell_bit;
    }

    friend std::ostream &operator<<(std::ostream &os, const Str &s) {
        return os << s.get_view();
    }

    friend std::istream &operator>>(std::istream &is, Str &s) {
        std::string seq;
        is >> seq;
        s.clear();
        s.append(seq);

        return is;
    }

    const_iterator begin() const { return const_iterator(data, 0); }
    const_iterator end() const { return const_iterator(data, length); }
};

} /* namespace triegraph */

#endif /* __STR_H__ */
