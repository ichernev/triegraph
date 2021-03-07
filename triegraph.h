#ifndef __TRIEGRAPH_H__
#define __TRIEGRAPH_H__

#include <utility>
#include <tuple>

// struct nodeloc {
//     u32 node;
//     u32 pos;
// };

template<typename Graph_, typename LetterLocData_, typename TrieData_>
struct Triegraph {
    using Graph = Graph_;
    using LetterLocData = LetterLocData_;
    using TrieData = TrieData_;
    using Size = typename Graph::Size;
    using NodeLoc = Size;
    using LetterLoc = Size;
    using Kmer = typename TrieData::Kmer;
    using NodePos = typename LetterLocData::NodePos;
    using TrieSize = typename TrieData::Kmer::Holder;
    using TrieDepth = typename TrieData::Kmer::klen_type;
    using Self = Triegraph;

    Graph graph;
    LetterLocData letter_loc;
    TrieData trie_data;

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


// using dfl_trigraph = trigraph<
//     rgfa_node<dna_str>,
//     rgfa_edge<typename dna_str::size_type>,
//     kmer<dna_letter, u64, 31>,
//     nodeloc>;

#endif /* __TRIEGRAPH_H__ */
