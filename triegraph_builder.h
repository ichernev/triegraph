#ifndef __TRIEGRAPH_BUILDER_H__
#define __TRIEGRAPH_BUILDER_H__

#include "util.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <type_traits>
#include <queue>
#include <limits>

template <typename Graph_, typename LetterLocData_, typename TrieData_, typename Triegraph_>
struct TriegraphBuilder {
    using Graph = Graph_;
    using Str = typename Graph::Str;
    using Size = typename Graph::Size;
    using NodeLoc = Size;
    using LetterLoc = Size; /* letter location */
    using LetterLocData = LetterLocData_;
    using NodePos = typename LetterLocData::NodePos;
    using TrieData = TrieData_;
    using Kmer = typename TrieData::Kmer;
    using Triegraph = Triegraph_;
    using kmer_len_type = u16; // assume no more than 64k kmers end in a given letter location

    Graph graph;
    LetterLocData letter_loc;
    TrieData trie_data;

    TriegraphBuilder(Graph &&graph) : graph(std::move(graph)) {}
    TriegraphBuilder(const TriegraphBuilder &) = delete;
    TriegraphBuilder &operator= (const TriegraphBuilder &) = delete;

    Triegraph build() {
        auto comps = ConnectedComp::build(graph);
        auto starts = _compute_starts(std::move(comps));
        letter_loc.init(graph);
        auto kmer_data = _bfs_trie(std::move(starts));
        _fill_trie_data(std::move(kmer_data));

        return Triegraph {
            std::move(graph),
            std::move(letter_loc),
            std::move(trie_data)
        };
    }

    struct ConnectedComp {
        const Graph &graph;
        std::vector<NodeLoc> comp_id; // (raw_graph.rgfa_nodes.size(), INV_SIZE);
        NodeLoc num_comp = 0;
        ConnectedComp(const Graph &graph)
            : graph(graph), comp_id(graph.nodes.size(), Graph::INV_SIZE), num_comp(0) {
        }

        static ConnectedComp build(const Graph &graph) {
            ConnectedComp comps(graph);
            for (NodeLoc i = 0; i < comps.comp_id.size(); ++i) {
                if (comps.comp_id[i] == Graph::INV_SIZE)
                    comps._bfs_2way(i, comps.num_comp++);
            }
            return comps;
        }

        void _bfs_2way(NodeLoc start, NodeLoc comp) {
            std::queue<Size> q;

            q.push(start);
            comp_id[start] = comp;

            while (!q.empty()) {
                auto crnt = q.front(); q.pop();

                for (const auto &to : graph.forward_from(crnt)) {
                    if (comp_id[to.node_id] == Graph::INV_SIZE) {
                        comp_id[to.node_id] = comp;
                        q.push(to.node_id);
                    }
                }
                for (const auto &from : graph.backward_from(crnt)) {
                    if (comp_id[from.node_id] == Graph::INV_SIZE) {
                        comp_id[from.node_id] = comp;
                        q.push(from.node_id);
                    }
                }
            }
        }
    };

    std::vector<NodeLoc> _compute_starts(ConnectedComp comps) const {
        std::vector<NodeLoc> starts;
        std::unordered_set<NodeLoc> done_comps;

        starts.reserve(comps.num_comp);
        for (NodeLoc i = 0; i < graph.nodes.size(); ++i) {
            if (auto bw = graph.backward_from(i); bw.begin() == bw.end()) {
                starts.push_back(i);
                done_comps.insert(comps.comp_id[i]);
            }
        }
        for (NodeLoc i = 0; i < graph.nodes.size(); ++i) {
            if (!done_comps.contains(comps.comp_id[i])) {
                done_comps.insert(comps.comp_id[i]);
                starts.push_back(i);
            }
        }
        return starts;
    }

    struct KmerBuildData {
        std::vector<std::vector<Kmer>> kmers;
        std::vector<kmer_len_type> done_idx;

        KmerBuildData(LetterLoc num) {
            // one more for fictional position to store kmers that match to end
            kmers.resize(num + 1);
            done_idx.resize(num + 1);
        }
    };

    KmerBuildData _bfs_trie(const std::vector<NodeLoc> &starts) {
        KmerBuildData kb(letter_loc.num_locations);

        std::queue<NodePos> q;

        for (auto start : starts) {
            auto h = NodePos(start, 0);
            q.push(h);
            kb.kmers[letter_loc.compress(h)].push_back(Kmer::empty());
        }

        while (!q.empty()) {
            auto h = q.front(); q.pop();
            auto hc = letter_loc.compress(h);
            if (hc == letter_loc.num_locations) {
                // this position signifies kmers that match to the end of the
                // graph
                kb.done_idx[hc] = kb.kmers[hc].size();
                continue;
            }

            std::vector<NodePos> targets;
            if (graph.nodes[h.node].seg.length == h.pos + 1) {
                for (const auto &to : graph.forward_from(h.node)) {
                    targets.push_back(NodePos(to.node_id, 0));
                }
            } else {
                targets.push_back(NodePos(h.node, h.pos+1));
            }

            if (targets.empty()) {
                // This signifies matching the end.
                // Needs support from LetterLocData::compress
                targets.push_back(NodePos(letter_loc.num_locations, 0));
            }

            auto &done_idx = kb.done_idx[hc];
            auto &kmers = kb.kmers[hc];
            auto letter = graph.nodes[h.node].seg[h.pos];
            for (; done_idx < kmers.size(); ++done_idx) {
                auto kmer = kmers[done_idx];
                kmer.push(letter);
                for (auto t: targets) {
                    auto tc = letter_loc.compress(t);
                    auto &t_kmers = kb.kmers[tc];
                    auto t_done = kb.done_idx[tc];
                    t_kmers.emplace_back(kmer);
                    if (t_done + 1u == t_kmers.size()) {
                        q.push(t);
                    }
                }
            }
        }
        return kb;
    }

    void _fill_trie_data(KmerBuildData kb) {
        for (LetterLoc i = 0; i <= letter_loc.num_locations; ++i) {
            if (kb.kmers[i].size() > std::numeric_limits<typename decltype(kb.done_idx)::value_type>::max()) {
                std::cerr << "got " << kb.kmers[i].size() << " kmers at " << i << std::endl;
                throw "too-many-kmers";
            }
            for (kmer_len_type j = 0; j < kb.done_idx[i]; ++j) {
                auto kmer = kb.kmers[i][j];
                if (kmer.is_complete()) {
                    // std::cerr << "full kmer " << kb.kmers[i][j] << " " << i << std::endl;
                    trie_data.trie2graph.emplace(kb.kmers[i][j], i);
                }
            }
        }

        // destroy kb
        { auto _ = std::move(kb); }

        // put a guard for kmer pop loop
        trie_data.active_trie.emplace(Kmer::empty());
        for (const auto &item : trie_data.trie2graph) {
            trie_data.graph2trie.emplace(item.second, item.first);
            auto kmer = item.first;
            do {
                kmer.pop();
            } while (trie_data.active_trie.emplace(kmer).second);
        }
    }
};

#endif /* __TRIEGRAPH_BUILDER_H__ */
