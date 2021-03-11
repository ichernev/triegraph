#ifndef __TRIEGRAPH_H__
#define __TRIEGRAPH_H__

#include <utility>
#include <tuple>

namespace triegraph {

template<typename TrieGraphData_>
struct TrieGraph {
    using TrieGraphData = TrieGraphData_;
    using Graph = TrieGraphData::Graph;
    using LetterLocData = TrieGraphData::LetterLocData;
    using TrieData = TrieGraphData::TrieData;
    using Size = typename Graph::Size;
    using NodeLoc = Size;
    using LetterLoc = Size;
    using Kmer = typename TrieData::Kmer;
    using NodePos = typename LetterLocData::NodePos;
    using TrieSize = typename TrieData::Kmer::Holder;
    using TrieDepth = typename TrieData::Kmer::klen_type;
    using Self = TrieGraph;

    TrieGraphData data;

    // std::tuple<NodeLoc, NodeLoc, LetterLoc> graph_size() const {
    //     return std::make_tuple(
    //             graph.nodes.size(),
    //             graph.edges.size() / 2,
    //             letter_loc.num_letters);
    // }

    // std::pair<TrieSize, TrieSize> trie_size() const {
    //     return std::make_pair(
    //             // NOTE: This is wrong
    //             trie_data.trie2graph.size(),
    //             trie_data.active_trie.size());
    // }

    // TrieDepth trie_depth() const {
    //     return TrieData::Kmer::K;
    // }

    // struct Handle {
    //     union {
    //         Kmer kmer;
    //         NodePos nodepos;
    //     };

    //     Handle(Kmer k) : kmer(k) {}
    //     Handle(NodeLoc n) : nodepos(n, 0) {}
    //     Handle(NodePos np) : nodepos(np) {}

    //     bool is_trie() const { return kmer.data & Kmer::ON_MASK; }
    //     TrieDepth depth() const { return kmer.get_len(); }

    //     bool is_graph() const { return !is_trie(); }
    //     NodeLoc node() const { return nodepos.node; }
    //     LetterLoc pos() const { return nodepos.pos; }
    // };

    // struct EditEdge {
    //     Handle handle;
    //     enum {MATCH, SUBST, INS, DEL} edit;
    //     Letter letter;
    // };


    // Handle root();

};

} /* namespace triegraph */

#endif /* __TRIEGRAPH_H__ */
