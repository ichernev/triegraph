#ifndef __HASH_POLICY_H__
#define __HASH_POLICY_H__

#include <unordered_map>
#include <unordered_set>

namespace triegraph {

template <typename Key, typename Val = Key>
struct HashPolicy {
    using Set = std::unordered_set<Key>;
    using Map = std::unordered_map<Key, Val>;
};

// #include <sparsepp/spp.h>

// namespace triegraph {

// template <typename Key, typename Val = Key>
// struct HashPolicy {
//     using Map = spp::sparse_hash_map<Key, Val>;
//     using Set = spp::sparse_hash_set<Key>;
// };

} /* namespace triegraph */

#endif /* __HASH_POLICY_H__ */
