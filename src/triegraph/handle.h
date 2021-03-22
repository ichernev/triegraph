#ifndef __HANDLE_H__
#define __HANDLE_H__

namespace triegraph {

template <typename Kmer_, typename NodePos_>
struct Handle {
    using Kmer = Kmer_;
    using NodePos = NodePos_;
    using NodeLoc = NodePos::NodeLoc;
    using NodeLen = NodePos::NodeLen;
    using TrieDepth = typename Kmer::klen_type;
    static constexpr NodeLen INV_NODE_LEN = std::numeric_limits<NodeLen>::max();

    static_assert(sizeof(Kmer) == 4 && sizeof(NodePos) == 8, "remove filler or rework");
    // TODO: THIS IS UGLYYY
    union {
        struct {
            u32  filler;
            Kmer kmer;
        };
        NodePos nodepos;
    };

    Handle() : nodepos(0, INV_NODE_LEN) {}
    Handle(Kmer k) : kmer(k) {}
    Handle(NodeLoc n, NodeLen p = 0) : nodepos(n, p) {}
    Handle(NodePos np) : nodepos(np) {}
    static Handle invalid() { return Handle(); }

    bool is_trie() const { return kmer.data & Kmer::ON_MASK; }
    TrieDepth depth_in_trie() const { return kmer.get_len(); }

    bool is_graph() const { return !is_trie(); }
    NodeLoc node() const { return nodepos.node; }
    NodeLen pos() const { return nodepos.pos; }

    bool is_valid() const { return !is_graph() || pos() != INV_NODE_LEN; }

    bool operator== (const Handle &other) const {
        if constexpr (sizeof(kmer) == sizeof(nodepos))
            return kmer == other.kmer;

        if (is_trie())
            return other.is_trie() ? kmer == other.kmer : false;
        else
            return other.is_graph() ? nodepos == other.nodepos : false;
    }

    friend std::ostream &operator<< (std::ostream &os, const Handle &h) {
        if (h.is_trie())
            os << "trie:" << h.depth_in_trie() << ":" << h.kmer;
        else
            os << "graph:" << h.node() << ":" << h.pos();
        return os;
    }
};

} /* namespace triegraph */

#endif /* __HANDLE_H__ */
