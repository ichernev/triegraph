#pragma once

using u32 = unsigned int;
using u64 = unsigned long long;
constexpr int BITS_PER_BYTE = 8;

constexpr int log2_ceil(u64 value) {
    int res = 0;
    while (value > 1) {
        res += 1;
        value /= 2;
    }
    return res;
}

constexpr u64 div_up(u64 a, u64 b) {
    return (a + b - 1) / b;
}

template <u64 options, typename holder, typename human_t, typename mapper, typename unmapper>
struct letter {
    static constexpr u64 num_options = options;
    static constexpr int bits = log2_ceil(options);
    static constexpr holder mask = (1 << bits) - 1;
    using holder_type = holder;
    using mapper_type = mapper;
    using unmapper_type = unmapper;
    using human_type = human_t;
};

struct dna_char_mapper {
    using holder_type = u32;
    using htype = char;
    holder_type operator()(htype letter) {
        switch (letter) {
            case 'A': case 'a':
                return 0;
            case 'C': case 'c':
                return 1;
            case 'G': case 'g':
                return 2;
            case 'T': case 't':
                return 3;
            default:
                throw "invalid letter";
        }
    }
};

struct dna_char_unmapper {
    using holder_type = u32;
    using htype = char;
    htype operator()(holder_type repr) {
        return "acgt"[repr];
    }
};

using dna_letter = letter<4, u32, char, dna_char_mapper, dna_char_unmapper>;

template <
    typename letter,
    typename store = typename letter::holder_type,
    typename sizet = u64>
struct str {
    using letter_type = letter;
    using letter_holder_type = typename letter::holder_type;
    using store_type = store;
    using size_type = sizet;
    using letter_human_type = typename letter::human_type;
    static constexpr int letter_bits = letter::bits;
    static constexpr int letters_per_holder = sizeof(letter_holder_type) * BITS_PER_BYTE / letter_bits;
    static constexpr int letters_per_store  = sizeof(store_type) * BITS_PER_BYTE / letter_bits;

    store_type *data;
    size_type length;   /* number of letters */
    size_type capacity; /* number of store_types */

    struct view {
        // using letter_holder_type = typename str::letter_holder_type;
        // using letter_type = typename str::letter_type;
        // using store_type = typename str::store_type;
        // using size_type = typename str::size_type;
        // using letter_htype = typename letter_type::human_type;

        const str *base;
        typename str::size_type offset;
        typename str::size_type length;
        str_view(const str *base_, str::size_type offset_, str::size_type length)
            : base(base_), offset(offset_), length(length_) {}

        letter_holder_type operator[](size_type idx) const { return (*base)[idx + offset]; }
        letter_holder_type at(size_type idx) const {
            if (idx < length) {
                return base->at(idx);
            }
            throw "out-of-bounds";
        }

        store_type _store(size_type idx) const {
            if (idx + offset % letters_per_holder != 0) {
                throw "incorrect-store-access";
            }
            return base->data[(idx+offset) / letters_per_holder];
        }

        size_type fast_match(const view &other) const {
            size_type mlen = std::min(length, other.length), i;
            for (i = 0; i < mlen && (*this)[i] == other[i]; ++i);
            return i;
        }

        size_type fast_match_2(const view &other) const {
            if ((offset - other.offset) % letters_per_store != 0) {
                return fast_match(other);
            }
            // do the first part slow
            int slow_search = (letters_per_store - offset % letters_per_store) % letters_per_store;
            size_type mlen = std::min(length, other.length), i;
            for (i = 0; i < slow_search && i < mlen && (*this)[i] == other[i]; ++i);
            if (i < slow_search && i < mlen) {
                return i;
            }
            // store by store
            for (i = slow_search; i < mlen && (*this)._store(i) == other._store(i); i += letters_per_store);
            // last part slow
            for (; i < mlen && (*this)[i] == other[i]; ++i);
            return i;
        }
    };

    str() {
        this->data = nullptr;
    }

    str(std::basic_string_view<letter_human_type> human)
        : data(nullptr), length(0), capacity(0)
    {
        append(human);
    }

    ~str() {
        free(this->data);
    }

    size_type append(std::basic_string_view<letter_human_type> human) {
        auto mapper = typename letter_type::mapper_type();

        size_type ncap = div_up(this->length + human.size(), letters_per_store);
        if (ncap > this->capacity) {
            this->capacity = ncap;
            this->data = realloc(this->data, ncap * sizeof(store_type));
        }

        store_type *oit = this->data + (this->length / letters_per_store);
        size_type sub_idx = this->length % letters_per_store;
        if (sub_idx == 0) *oit = 0;
        for (auto iit = human.begin(); iit != human.end(); ++iit, ++this->length) {
            if (sub_idx == letters_per_store) {
                sub_idx = 0;
                oit += 1;
                *oit = 0;
            }
            letter_holder_type bin = mapper(*iit);
            *oit |= bin << (sub_idx * letter_type::bits);
            sub_idx += 1;
        }
    }

    view get_view(size_type offset, size_type length) {
        return view(this, offset, length);
    }
    view get_view(size_type offset) {
        return get_view(offset, length - offset);
    }

    letter_holder_type operator[](size_type idx) const {
        size_type cell = idx / letters_per_store;
        int cell_bit = (idx % letters_per_store) * letter_type::bits;

        return static_cast<letter_holder_type>((this->data[cell] >> cell_bit) & letter_type::mask);
    }

    void set(size_type index, letter_holder_type val) {
        size_type cell = idx / letters_per_store;
        int cell_bit = (idx % letters_per_store) * letter_type::bits;
        this->data[cell] &= letter_type::mask << cell_bit;
        this->data[cell] |= val << cell_bit;
    }

    letter_holder_type at(size_type idx) const {
        if (idx < length) {
            return (*this)[idx];
        }
        throw "out-of-bounds";
    }
};
