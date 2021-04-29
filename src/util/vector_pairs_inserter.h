#ifndef __UTIL_VECTOR_PAIRS_INSERTER_H__
#define __UTIL_VECTOR_PAIRS_INSERTER_H__

namespace triegraph {

template <typename VectorPairs,
         typename FirstMapper,
         typename SecondMapper,
         typename value_type_>
struct VectorPairsInserter {
    using value_type = value_type_;
    VectorPairs &pairs;
    FirstMapper fmap;
    SecondMapper smap;
    VectorPairsInserter(VectorPairs &pairs, FirstMapper fmap, SecondMapper smap)
        : pairs(pairs),
          fmap(fmap),
          smap(smap)
    {}

    void reserve(size_t capacity) { pairs.reserve(capacity); }

    void emplace_back(auto &&a, auto &&b) {
        pairs.emplace_back(
                fmap(std::forward<decltype(a)>(a)),
                smap(std::forward<decltype(b)>(b)));
    }

    void push_back(const auto &p) {
        pairs.emplace_back(fmap(p.first), smap(p.second));
    }
};


} /* namespace triegraph */

#endif /* __UTIL_VECTOR_PAIRS_INSERTER_H__ */
