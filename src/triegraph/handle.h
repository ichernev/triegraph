// SPDX-License-Identifier: MPL-2.0
/*
 * Copyright (c) 2021, Iskren Chernev
 */

#ifndef __HANDLE_H__
#define __HANDLE_H__

#include <variant>

namespace triegraph {

template <typename Kmer_, typename NodePos_>
struct Handle {
    using Kmer = Kmer_;
    using NodePos = NodePos_;
    using NodeLoc = NodePos::NodeLoc;
    using NodeLen = NodePos::NodeLen;
    using TrieDepth = typename Kmer::klen_type;
    static constexpr NodeLen INV_NODE_LEN = std::numeric_limits<NodeLen>::max();

    enum struct StateId : u32 { Inval = 0, Kmer, NodePos };
    std::variant<std::monostate, Kmer, NodePos> state;

    Handle() {}
    Handle(Kmer k) : state(k) {}
    Handle(NodeLoc n, NodeLen p = 0) : state(NodePos{n, p}) {}
    Handle(NodePos np) : state(np) {}

    static Handle invalid() { return Handle(); }

    bool is_trie() const { return state.index() == static_cast<u32>(StateId::Kmer); }
    Kmer &kmer() { return std::get<Kmer>(state); }
    const Kmer &kmer() const { return std::get<Kmer>(state); }
    TrieDepth depth_in_trie() const { return kmer().get_len(); }

    bool is_graph() const { return state.index() == static_cast<u32>(StateId::NodePos); }
    NodePos &nodepos() { return std::get<NodePos>(state); }
    const NodePos &nodepos() const { return std::get<NodePos>(state); }
    NodeLoc node() const { return nodepos().node; }
    NodeLen pos() const { return nodepos().pos; }

    bool is_valid() const { return state.index() != static_cast<u32>(StateId::Inval); }

    bool operator== (const Handle &other) const {
        return state == other.state;
    }

    bool operator< (const Handle &other) const {
        if (is_trie())
            return other.is_trie() ? kmer() < other.kmer() : true;
        else
            return other.is_graph() ? nodepos() < other.nodepos() : false;
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

namespace std {
    template <typename Kmer, typename NodePos>
    struct hash<triegraph::Handle<Kmer, NodePos>> {
        size_t operator() (const triegraph::Handle<Kmer, NodePos> &h) const {
            return !h.is_valid() ? 0 :
                h.is_trie() ? std::hash<Kmer>{}(h.kmer()) :
                    std::hash<NodePos>{}(h.nodepos());
        }
    };
} /* namespace std */

#endif /* __HANDLE_H__ */
