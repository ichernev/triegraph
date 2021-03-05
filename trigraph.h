#pragma once

#include "str.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <type_traits>
#include <queue>

template<typename letter, typename holder, unsigned k>
struct kmer {
    using holder_type = holder;
    using letter_type = letter;
    using klen_type = unsigned;
    // use one bit to encode less-than-maxk length
    static constexpr holder_type H1 = 1;
    static constexpr klen_type MAX_K = (sizeof(holder) * BITS_PER_BYTE - 1) / letter::bits;
    static constexpr klen_type K = k;
    static constexpr holder_type _kmer_mask(klen_type anti_len) {
        return (H1 << (K - anti_len) * letter::bits) - 1;
    }
    static constexpr holder_type KMER_MASK = _kmer_mask(0);
    static constexpr holder_type L1_MASK = H1 << MAX_K * letter::bits;
    static constexpr holder_type L2_MASK = holder_type(letter::mask);
    static constexpr holder_type L2_SHIFT = (MAX_K - 1) * letter::bits;
    static constexpr holder_type L3_MASK = (H1 << 3 * letter::bits) - 1;
    static constexpr holder_type L3_SHIFT = MAX_K * letter::bits - 4;
    static constexpr holder_type EMPTY = L1_MASK | L2_MASK << L2_SHIFT | K-4 << L3_SHIFT;
    static_assert(std::is_unsigned_v<holder_type>, "holder should be unsigned");
    static_assert(k <= MAX_K, "can not support big k, increase holder size");

    holder_type data;

    static kmer empty() {
        return kmer {EMPTY};
    }

    bool is_complete() const {
        return data & L1_MASK == 0;
    }

    klen_type get_len() const {
        if (is_complete())
            return K;

        holder_type l2 = data >> L2_SHIFT & L2_MASK;
        if (l2 == L2_MASK) {
            holder_type l3 = data >> L3_SHIFT & L3_MASK;
            return K - (1 + L2_MASK + l3);
        } else {
            return K - (1 + l2);
        }
    }

    void push(typename letter::holder_type l) {
        if (is_complete()) {
            // NOTE: this doesn't preserve "waste" bits
            data <<= letter::bits;
            data |= l /* & letter::bits */;
            data &= KMER_MASK;
        } else {
            holder_type mask = _inc_len();
            data = data & ~mask | (data << letter::bits | l) & mask;
        }
    }

    void pop() {
        holder_type mask = _dec_len();
        data = data & ~mask | (data >> letter::bits) & mask;
    }

    holder_type _l2_get() const { return data >> L2_SHIFT & L2_MASK; }
    holder_type _l3_get() const { return data >> L3_SHIFT & L3_MASK; }
    void _l2_set(holder_type l2) {
        data &= ~(L2_MASK << L2_SHIFT);
        data |= l2 << L2_SHIFT;
    }
    void _l3_set(holder_type l3) {
        data &= ~(L3_MASK << L3_SHIFT);
        data |= l3 << L3_SHIFT;
    }

    // returns the mask to contain the old
    holder_type _inc_len() {
        holder_type l2 = _l2_get();
        if (l2 == L2_MASK) {
            holder_type l3 = _l3_get();
            if (l3 == 0) {
                goto decr_l2;
            } else {
                _l3_set(--l3);
                return _kmer_mask(1 + l2 + l3);
            }
        } else {
            if (l2 == 0) {
                data ^= L1_MASK;
                return KMER_MASK;
            }
decr_l2:
            _l2_set(--l2);
            return _kmer_mask(1 + l2);
        }
    }

    holder_type _dec_len() {
        if (is_complete()) {
            data |= L1_MASK;
            _l2_set(0);

            return _kmer_mask(1);
        }
        holder_type l2 = _l2_get();
        if (l2 == L2_MASK) {
            holder_type l3 = _l3_get();

            if (1 + l2 + l3 == K)
                // already empty
                return 0;

            _l3_set(++l3);
            return _kmer_mask(1 + l2 + l3);
        } else {
            _l2_set(++l2);
            return _kmer_mask(1 + l2);
        }
    }
};

namespace std {
template<typename letter, typename holder, unsigned k>
struct hash<kmer<letter, holder, k>> {
    using hasher_t = hash<holder>;
    static constexpr hasher_t hasher();
    std::size_t operator() (const kmer<letter, holder, k> &kmr) {
        return hasher(kmr.data);
    }
};
}

struct nodeloc {
    u32 node;
    u32 pos;
};

template<typename rgfa_node_t, typename rgfa_edge_t, typename kmer_t, typename handle_t>
struct trigraph {
    using size_type = typename rgfa_node_t::size_type;
    using str_type = typename rgfa_node_t::str_type;
    using handle_type = handle_t;
    using kmer_type = kmer_t;
    using kmer_len_type = unsigned short; // assume no more than 64k kmers end in given location
    static constexpr size_type INV_SIZE = -1;

    struct kmer_comp_t {
        std::vector<std::vector<kmer_type>> kmers;
        std::vector<kmer_len_type> done_idx;

        kmer_comp_t(size_type num) {
            kmers.resize(num);
            done_idx.resize(num);
        }
    };

    struct trie_data_t {
        std::unordered_multimap<kmer_type, size_type> fw;
        std::unordered_multimap<size_type, kmer_type> bw;

        std::unordered_set<kmer_type> active;
    };

    struct location_data_t {
        std::vector<size_type> node_start;
        size_type num_locations;

        location_data_t(const raw_graph_t &rg) {
            node_start.reserve(rg.rgfa_nodes.size() + 1);
            num_locations = 0;
            for (const auto &node: rg.rgfa_nodes) {
                node_start.push_back(num_locations);
                num_locations += node.seg.length;
            }
            node_start.push_back(num_locations);
        }

        size_type loc2node(size_type loc) {
            return std::lower_bound(node_start.begin(), node_start.end(), loc) - node_start.begin();
        }

        // size_type node_start(size_type node_id) {
        //     return node_start[node_id];
        // }

        handle_type expand(size_type loc) {
            size_type node = loc2node(loc);
            return handle_type(node, loc - node_start[node]);
        }

        size_type compress(handle_type hnd) {
            return node_start[hnd.node] + hnd.pos;
        }
    };

    raw_graph_t raw_graph;
    trie_data_t trie_data;
    // std::vector<size_type> node_id;

    trigraph(raw_graph_t &&rg)
        : raw_graph(std::move(rg)) {
    }


    struct component_data_t {
        std::vector<size_type> comp_id; // (raw_graph.rgfa_nodes.size(), INV_SIZE);
        size_type num_comp = 0;
        component_data_t(size_type num_nodes)
            : comp_id(num_nodes, INV_SIZE), num_comp(0) {
        }
    };

    void compute_trie(int depth) {
        auto comps = _compute_components(raw_graph);
        auto starts = _compute_starts(raw_graph, std::move(comps));
        location_data_t locs(raw_graph);
        auto kmer_data = _bfs_trie(raw_graph, locs, std::move(starts));
        trie_data = _fill_trie(std::move(kmer_data));
    }

    static component_data_t _compute_components(const raw_graph_t &rg) {
        component_data_t comps(rg.rgfa_nodes.size());
        for (size_type i = 0; i < comps.comp_id.size(); ++i) {
            if (comps.comp_id[i] == INV_SIZE)
                _bfs_2way(rg, comps.comp_id, i, comps.num_comp++);
        }
        return comps;
    }

    static std::vector<size_type> _compute_starts(const raw_graph_t &rg, component_data_t comps) {
        std::vector<size_type> starts;
        std::unordered_set<size_type> done_comps;

        starts.reserve(comps.num_comp);
        for (size_type i = 0; i < rg.rgfa_nodes.size(); ++i) {
            if (rg.redge_start[i] == INV_SIZE) {
                starts.push_back(i);
                done_comps.insert(comps.comp_id[i]);
            }
        }
        for (size_type i = 0; i < rg.rgfa_nodes.size(); ++i) {
            if (!done_comps.contains(comps.comp_id[i])) {
                done_comps.insert(comps.comp_id[i]);
                starts.push_back(i);
            }
        }
        return starts;
    }

    static void _bfs_2way(const raw_graph_t &rg, std::vector<size_type> &comp_id,
            size_type start, size_type comp) {
        std::queue<size_type> q;

        q.push(start);
        comp_id[start] = comp;

        while (!q.empty()) {
            size_type crnt = q.pop();

            // fwd edges
            for (size_type edge_id = rg.edge_start[crnt]; edge_id != INV_SIZE;
                    edge_id = rg.rgfa_edges[edge_id].next) {
                size_type to = rg.rgfa_edges[edge_id].to;
                if (comp_id[to] == INV_SIZE) {
                    comp_id[to] = comp;
                    q.push(to);
                }
            }
            // rev edges
            for (size_type edge_id = rg.redge_start[crnt]; edge_id != INV_SIZE;
                    edge_id = rg.rgfa_edges[edge_id].next) {
                size_type to = rg.rgfa_edges[edge_id].to;
                if (comp_id[to] == INV_SIZE) {
                    comp_id[to] = comp;
                    q.push(to);
                }
            }
        }
    }

    static kmer_comp_t _bfs_trie(const raw_graph_t &rg, const location_data_t &locs,
            const std::vector<size_type> &starts) {
        kmer_comp_t kc(locs.num_locations);

        std::queue<handle_type> q;

        for (auto start : starts) {
            handle_type h(start, 0);
            q.push(h);
            kc.kmers[locs.compress(h)].push_back(kmer_type::empty());
        }

        while (!q.empty()) {
            handle_type h = q.pop();
            auto hc = locs.compress(h);
            auto &done_idx = kc.done_idx[hc];
            auto &kmers = kc.kmers[hc];
            typename rgfa_node_t::str_type::letter_type::holder_type l = rg.rgfa_nodes[h.node].seg[h.pos];

            std::vector<handle_type> to;
            if (rg.node_len(h.node) == h.pos - 1) {
                for (size_type edge_id = rg.edge_start[h.node]; edge_id != INV_SIZE;
                        edge_id = rg.rgfa_edges[edge_id].next) {
                    to.push_back(handle_type(rg.rgfa_edges[edge_id].to, 0));
                }
            } else {
                to.push_back(handle_type(h.node, h.pos+1));
            }

            for (; done_idx < kmers.size(); ++done_idx) {
                auto kmer = kmers[done_idx];
                kmer.push(l);
                for (auto t: to) {
                    auto tc = locs.compress(t);
                    auto &t_kmers = kc.kmers[tc];
                    auto &t_done = kc.done_idx[tc];
                    t_kmers.push_back(kmer);
                    if (t_done + 1 == t_kmers.size()) {
                        q.push(t);
                    }
                }
            }
        }
        return kc;
    }

    static trie_data_t _fill_trie(kmer_comp_t kc, const location_data_t &locs) {
        trie_data_t td;

        for (size_type i = 0; i < locs.num_letters; ++i) {
            for (kmer_len_type j = 0; j < kc.done_idx[i]; ++j) {
                td.fw.emplace(kc.kmers[i][j], i);
            }
        }

        // destroy kc
        { auto _ = std::move(kc); }

        // put a guard for kmer pop loop
        td.active.emplace(kmer_type::empty());
        for (const auto &item : td.fw) {
            td.bw.emplace(item.second, item.first);
            auto kmer = item.first;
            do {
                kmer.pop();
            } while (td.active.emplace(kmer).second);
        }

        return td;
    }
};


using dfl_trigraph = trigraph<
    rgfa_node<dna_str>,
    rgfa_edge<typename dna_str::size_type>,
    kmer<dna_letter, u64, 31>,
    nodeloc>;
