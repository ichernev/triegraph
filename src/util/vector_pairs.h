#ifndef __UTIL_VECTOR_PAIRS_H__
#define __UTIL_VECTOR_PAIRS_H__

#include <util/util.h>
#include <functional>

namespace triegraph {

enum struct VectorPairsImpl : u32 { EMPTY, SIMPLE, DUAL };

template <typename T1, typename T2, VectorPairsImpl impl_choice>
struct VectorPairs {

    using Self = VectorPairs;

    using fwd_pair = std::pair<T1, T2>;
    using rev_pair = std::pair<T2, T1>;
    using fwd_vec = std::vector<fwd_pair>;
    using rev_vec = std::vector<rev_pair>;

    using value_type = fwd_pair; // for back_inserter

    struct Empty {
        size_t size() const { return 0; }
        void reserve(size_t capacity) { }

        template <typename Tx1, typename Tx2>
        void emplace_back(Tx1 &&a, Tx2 &&b) {}
        void push_back(const fwd_pair &) {}

        void sort_by_fwd() {}
        void sort_by_rev() {}
        void unique() {}

        void write_to_disk() {}
        // FSStreamer stream_from_disk() {}
        void sort_on_disk() {}

        using const_fwd_view = iter_pair<
            typename fwd_vec::const_iterator,
            typename fwd_vec::const_iterator>;
        const_fwd_view fwd_pairs() const {
            return {
                typename const_fwd_view::iterator(nullptr),
                typename const_fwd_view::sentinel(nullptr)
            };
        }
        using const_rev_view = iter_pair<
            typename rev_vec::const_iterator,
            typename rev_vec::const_iterator>;
        const_rev_view rev_pairs() const {
            return {
                typename const_rev_view::iterator(nullptr),
                typename const_rev_view::sentinel(nullptr)
            };
        }
    };

    struct Simple {
        fwd_vec vec;

        size_t size() const { return vec.size(); }
        void reserve(size_t cap) { vec.reserve(cap); }

        template <typename Tx1, typename Tx2>
        void emplace_back(Tx1 &&a, Tx2 &&b) {
            vec.emplace_back(std::forward<Tx1>(a), std::forward<Tx2>(b));
        }
        void push_back(const fwd_pair &p) { vec.push_back(p); }
        void push_back(fwd_pair &&p) { vec.push_back(std::move(p)); }

        void sort_by_fwd() {
            std::ranges::sort(vec);
        }

        void sort_by_rev() {
            std::ranges::sort(vec, [](const auto &a, const auto &b) {
                return a.second != b.second ? a.second < b.second : a.first < b.first;
            });
        }

        void unique() {
            auto ur = std::ranges::unique(vec);
            vec.resize(ur.begin() - vec.begin());
        }


        using const_fwd_view = iter_pair<
            typename fwd_vec::const_iterator,
            typename fwd_vec::const_iterator>;
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

    struct Dual {
        std::vector<T1> vec1;
        std::vector<T2> vec2;
    };

    using Impl = choose_type_t<u32(impl_choice), Empty, Simple, Dual>;
    Impl impl;

    // struct FSStreamer {
    //     auto fwd_pairs();
    //     auto rev_pairs();
    // };

    size_t size() const { return impl.size(); }
    void reserve(size_t cap) { impl.reserve(cap); }

    template <typename Tx1, typename Tx2>
    void emplace_back(Tx1 &&a, Tx2 &&b) {
        impl.emplace_back(std::forward<Tx1>(a), std::forward<Tx2>(b));
    }
    void push_back(const fwd_pair &p) { impl.push_back(p); }
    void push_back(fwd_pair &&p) { impl.push_back(std::move(p)); }

    Self &sort_by_fwd() { impl.sort_by_fwd(); return *this; }
    Self &sort_by_rev() { impl.sort_by_rev(); return *this; }
    Self &unique() { impl.unique(); return *this; }

    // void write_to_disk() {}
    // FSStreamer stream_from_disk() {}
    // void sort_on_disk() {}

    auto fwd_pairs() const { return impl.fwd_pairs(); }
    auto rev_pairs() const { return impl.rev_pairs(); }
};

} /* namespace triegraph */

#endif /* __UTIL_VECTOR_PAIRS_H__ */
