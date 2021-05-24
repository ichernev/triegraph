#ifndef __UTIL_VECTOR_PAIRS_H__
#define __UTIL_VECTOR_PAIRS_H__

#include <util/util.h>
#include <functional>

namespace triegraph {

enum struct VectorPairsImpl : u32 { EMPTY = 0, SIMPLE = 1, DUAL = 2 };

template <typename T1_, typename T2_, VectorPairsImpl impl_choice>
struct VectorPairsBase {
    using T1 = T1_;
    using T2 = T2_;
    using fwd_pair = std::pair<T1, T2>;
    using rev_pair = std::pair<T2, T1>;
    // These are used in tests to build vectors for comparison with VP
    using fwd_vec = std::vector<fwd_pair>;
    using rev_vec = std::vector<rev_pair>;
    using value_type = fwd_pair; // for back_inserter
    static constexpr VectorPairsImpl impl = impl_choice;
};

template <typename T1, typename T2>
struct VectorPairsEmpty : public VectorPairsBase<T1, T2, VectorPairsImpl::EMPTY> {
    using Base = VectorPairsBase<T1, T2, VectorPairsImpl::EMPTY>;
    using Self = VectorPairsEmpty;

    size_t size() const { return 0; }
    void reserve(size_t capacity) { }

    template <typename Tx1, typename Tx2>
    void emplace_back(Tx1 &&a, Tx2 &&b) {}
    void push_back(const Base::fwd_pair &) {}

    Self &sort_by_fwd() { return *this; }
    Self &sort_by_rev() { return *this; }
    Self &unique() { return *this; }

    // void write_to_disk() {}
    // // FSStreamer stream_from_disk() {}
    // void sort_on_disk() {}

    using const_fwd_view = iter_pair<
        typename Base::fwd_vec::const_iterator,
        typename Base::fwd_vec::const_iterator>;
    const_fwd_view fwd_pairs() const {
        return {
            typename const_fwd_view::iterator(nullptr),
            typename const_fwd_view::sentinel(nullptr)
        };
    }
    using const_rev_view = iter_pair<
        typename Base::rev_vec::const_iterator,
        typename Base::rev_vec::const_iterator>;
    const_rev_view rev_pairs() const {
        return {
            typename const_rev_view::iterator(nullptr),
            typename const_rev_view::sentinel(nullptr)
        };
    }
};

template <typename T1, typename T2>
struct VectorPairsSimple : public VectorPairsBase<T1, T2, VectorPairsImpl::SIMPLE> {
    using Self = VectorPairsSimple;
    using Base = VectorPairsBase<T1, T2, VectorPairsImpl::SIMPLE>;
    Base::fwd_vec vec;

    size_t size() const { return vec.size(); }
    void reserve(size_t cap) { vec.reserve(cap); }

    template <typename Tx1, typename Tx2>
    void emplace_back(Tx1 &&a, Tx2 &&b) {
        vec.emplace_back(std::forward<Tx1>(a), std::forward<Tx2>(b));
    }
    void push_back(const Base::fwd_pair &p) { vec.push_back(p); }
    void push_back(Base::fwd_pair &&p) { vec.push_back(std::move(p)); }

    Self &sort_by_fwd() {
        std::ranges::sort(vec);
        return *this;
    }

    Self &sort_by_rev() {
        std::ranges::sort(vec, [](const auto &a, const auto &b) {
            return a.second != b.second ? a.second < b.second : a.first < b.first;
        });
        return *this;
    }

    Self &unique() {
        auto ur = std::ranges::unique(vec);
        vec.resize(ur.begin() - vec.begin());
        return *this;
    }

    using const_fwd_view = iter_pair<
        typename Base::fwd_vec::const_iterator,
        typename Base::fwd_vec::const_iterator>;
    const_fwd_view fwd_pairs() const {
        return {vec.begin(), vec.end()};
    }

    auto rev_pairs() const {
        return fwd_pairs() | std::ranges::views::transform([](const auto &a) {
                return std::pair<T2, T1>(a.second, a.first);
        });
    }
    // I can't make it compile...
    // using const_rev_view = std::result_of<
    //     decltype(&Simple::rev_pairs)(const Simple *)>;
};

namespace impl {

    template <typename A, typename B>
    std::strong_ordering _cmp(A &&a, B &&b) {
        if (const auto cmp = a.first <=> b.first; cmp != 0)
            return cmp;
        return a.second <=> b.second;
    }
    template <typename A, typename B>
    bool _eq(A &&a, B &&b) { return a.first == b.first && a.second == b.second; }
    template <typename A, typename B>
    void _copy(A &&a, B &&b) { a.first = b.first; a.second = b.second; }
    template <typename A, typename B>
    void _swap(A &&a, B &&b) { std::swap(a.first, b.first); std::swap(a.second, b.second); }

    template <typename T1, typename T2>
    struct Pair {
        T1 first;
        T2 second;

        Pair(const T1 &first, const T2 &second) : first(first), second(second) {}
        Pair(const Pair &) = default;
        Pair(auto &&other) { _copy(*this, other); }

        friend std::strong_ordering operator<=> (const Pair &a, const Pair &b) {
            return _cmp(a, b);
        }
        friend bool operator== (const Pair &a, const Pair &b) { return _eq(a, b); }
        friend void swap(Pair &a, Pair &b) { _swap(a, b); }
    };

    template <typename T1, typename T2, typename R1 = T1&, typename R2 = T2&>
    struct RefPair {
        R1 first;
        R2 second;
        using Pair = impl::Pair<T1, T2>;

        RefPair(R1 a, R2 b) : first(a), second(b) {}
        // TODO: Add R1&&, R2&&
        RefPair &operator= (RefPair &&other) { _copy(*this, other); return *this; }
        RefPair &operator= (Pair &&other) { _copy(*this, other); return *this; }
        friend std::strong_ordering operator<=> (const RefPair &a, const Pair &b) {
            return _cmp(a, b);
        }
        friend std::strong_ordering operator<=> (const RefPair &a, const RefPair &b) {
            return _cmp(a, b);
        }
        friend std::strong_ordering operator<=> (const Pair &a, const RefPair &b) {
            return _cmp(a, b);
        }
        friend bool operator== (const RefPair &a, const Pair &b) { return _eq(a, b); }
        friend bool operator== (const RefPair &a, const RefPair &b) { return _eq(a, b); }
        friend bool operator== (const Pair &a, const RefPair &b) { return _eq(a, b); }
        friend void swap(RefPair a, RefPair b) { _swap(a, b); }

        operator Pair() const { return Pair(first, second); }
    };

    template <typename T1, typename T2, bool is_const,
             typename v1_iter, typename v2_iter>
    struct PairIter {
        using Self = PairIter;

        v1_iter beg1;
        v2_iter beg2;

        v1_iter it1_;

        PairIter(v1_iter beg1 = {}, v2_iter beg2 = {}, v1_iter it = {})
            : beg1(beg1),
              beg2(beg2),
              it1_(it)
        {}
        PairIter(const Self &) = default;
        PairIter(Self &&) = default;
        Self &operator= (const Self &) = default;
        Self &operator= (Self &&) = default;

        std::size_t idx() const { return it1_ - beg1; }

        v1_iter &it1() { return it1_; }
        v1_iter it1() const { return it1_; }
        v2_iter it2() const { return beg2 + idx(); }

        using iterator_category = std::random_access_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = std::conditional_t<is_const,
              std::pair<T1, T2>, Pair<T1, T2>>;
        using reference = std::conditional_t<is_const,
              std::pair<T1, T2>, RefPair<T1, T2, typename v1_iter::reference,
                                                 typename v2_iter::reference>>;

        reference operator* () const { return reference(*it1(), *it2()); }
        Self &operator++ () { ++it1_; return *this; }
        Self &operator-- () { --it1_; return *this; }
        Self operator++ (int) { Self tmp = *this; ++(*this); return tmp; }
        Self operator-- (int) { Self tmp = *this; --(*this); return tmp; }
        bool operator== (const Self &other) const { return it1_ == other.it1_; }
        std::strong_ordering operator<=> (const Self &other) const { return it1_ <=> other.it1_; }

        Self &operator+= (difference_type i) { it1_ += i; return *this; }
        Self &operator-= (difference_type i) { it1_ -= i; return *this; }
        Self operator+ (difference_type i) const { Self tmp = *this; tmp += i; return tmp; }
        Self operator- (difference_type i) const { Self tmp = *this; tmp -= i; return tmp; }
        friend Self operator+ (difference_type i, const Self &it) { Self tmp = it; tmp += i; return tmp; }

        reference operator[] (difference_type i) const { return reference(*(it1()+i), *(it2()+i)); }
        difference_type operator- (const Self &other) const { return it1_ - other.it1_; }
    };


} /* namespace impl */


template <typename T1, typename T2,
         typename V1 = std::vector<T1>,
         typename V2 = std::vector<T2>>
struct VectorPairsDual : public VectorPairsBase<T1, T2, VectorPairsImpl::DUAL> {
    using Self = VectorPairsDual;
    using Base = VectorPairsBase<T1, T2, VectorPairsImpl::DUAL>;

    V1 vec1;
    V2 vec2;

    size_t size() const { return vec1.size(); }
    void reserve(size_t cap) { vec1.reserve(cap); vec2.reserve(cap); }

    template <typename Tx1, typename Tx2>
    void emplace_back(Tx1 &&a, Tx2 &&b) {
        vec1.emplace_back(std::forward<Tx1>(a));
        vec2.emplace_back(std::forward<Tx2>(b));
    }
    void push_back(const impl::Pair<T1, T2> &p) {
        vec1.push_back(p.first);
        vec2.push_back(p.second);
    }
    void push_back(const std::pair<T1, T2> &p) {
        vec1.push_back(p.first);
        vec2.push_back(p.second);
    }

    Self &sort_by_fwd() {
        using PI = impl::PairIter<T1, T2, false, typename V1::iterator, typename V2::iterator>;
        std::sort(
                PI(vec1.begin(), vec2.begin(), vec1.begin()),
                PI(vec1.begin(), vec2.begin(), vec1.end()));
        return *this;
    }
    Self &sort_by_rev() {
        using PI = impl::PairIter<T2, T1, false, typename V2::iterator, typename V1::iterator>;
        std::sort(
                PI(vec2.begin(), vec1.begin(), vec2.begin()),
                PI(vec2.begin(), vec1.begin(), vec2.end()));
        return *this;
    }
    Self &unique() {
        using PI = impl::PairIter<T1, T2, false, typename V1::iterator, typename V2::iterator>;
        auto beg = PI(vec1.begin(), vec2.begin(), vec1.begin()),
             end = PI(vec1.begin(), vec2.begin(), vec1.end());
        auto ue = std::unique(beg, end);
        vec1.resize(ue - beg);
        vec2.resize(ue - beg);
        return *this;
    }

    auto fwd_pairs() const {
        using PI = impl::PairIter<T1, T2, true, typename V1::const_iterator, typename V2::const_iterator>;
        return iter_pair<PI, PI> {
            PI(vec1.begin(), vec2.begin(), vec1.begin()),
            PI(vec1.begin(), vec2.begin(), vec1.end())
        };
    }
    auto rev_pairs() const {
        using PI = impl::PairIter<T2, T1, true, typename V2::const_iterator, typename V1::const_iterator>;
        return iter_pair<PI, PI> {
            PI(vec2.begin(), vec1.begin(), vec2.begin()),
            PI(vec2.begin(), vec1.begin(), vec2.end())
        };
    }

    V1 &get_v1() { return vec1; }
    V2 &get_v2() { return vec2; }
    V1 take_v1() { V1 res; std::swap(res, vec1); return res; }
    V2 take_v2() { V2 res; std::swap(res, vec2); return res; }
};

} /* namespace triegraph */

#endif /* __UTIL_VECTOR_PAIRS_H__ */
