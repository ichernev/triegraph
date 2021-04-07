#ifndef __TRIEGRAPH_BUILDER_NBFS_H__
#define __TRIEGRAPH_BUILDER_NBFS_H__

#include <graph/top_order.h>
#include <util/util.h>
#include <util/logger.h>

#include <vector>
#include <queue>
#include <utility>

namespace triegraph {

/**
 * Node BFS -- traverse nodes with a modified BFS using a priority queue sorted
 * by Topological Order.
 *
 * For each node, use kmer rolling to compute all kmers along the letter
 * segment.
 */
template <typename Graph_,
         typename LetterLocData_,
         typename Kmer_,
         typename KmerPerNodeLen_ = Kmer_::Holder>
struct TrieGraphBuilderNBFS {
    using Graph = Graph_;
    using LetterLocData = LetterLocData_;
    using NodeLoc = Graph::NodeLoc;
    using NodePos = LetterLocData::NodePos;
    using NodeLen = LetterLocData::NodeLen;
    using LetterLoc = LetterLocData::LetterLoc;
    using Kmer = Kmer_;
    using KmerPerNodeLen = KmerPerNodeLen_;
    using Str = Graph::Str;
    using TopOrder = triegraph::TopOrder<Graph>;
    using pairs_t = std::vector<std::pair<Kmer, LetterLoc>>;

    const Graph &graph;
    const LetterLocData &lloc;

    TopOrder top_ord;
    std::priority_queue<NodeLoc, std::vector<NodeLoc>, typename TopOrder::Comparator> q;
    std::vector<bool> in_q;
    pairs_t pairs;

    TrieGraphBuilderNBFS(const Graph &graph, const LetterLocData &lloc)
        : graph(graph),
          lloc(lloc),
          top_ord(typename TopOrder::Builder(graph).build()),
          q(top_ord.comp()),
          in_q(graph.num_nodes(), false),
          kd(graph.num_nodes())
    {}

    pairs_t get_pairs(std::ranges::range auto const& /*ignored*/, u32 /*ignored*/) && {
        auto scope = Logger::get().begin_scoped("node_bfs builder");

        auto starts = ConnectedComponents(graph).compute_starting_points();
        for (const auto &node : starts) {
            in_q[node] = true;
            q.push(node);
            kd.add_kmer(node, Kmer::empty());
            // std::cerr << "start: Pushing node " << node << std::endl;
        }

        _bfs();

        return std::move(pairs);
    }

private:
    /**
     * This is similar to KmerBuildData from BFS builder, but this one stores
     * kmers for each graph node, where as the other one stores kmers for every
     * letter location.
     *
     * It could be shared, the code is exactly the same.
     */
    struct KmerBuildData {

        static constexpr u32 SET_CUTOFF = 500;
        std::vector<compressed_vector<Kmer>> kmers;
        // std::vector<std::vector<Kmer>> kmers;
        std::vector<std::unordered_set<Kmer>> kmers_set;
        std::vector<KmerPerNodeLen> done_idx;

        // Stats &stats;

        KmerBuildData(NodeLoc num_nodes)
            : kmers(num_nodes),
              kmers_set(num_nodes),
              done_idx(num_nodes)
        {}

        bool exists(NodeLoc node, Kmer kmer) const {
            auto &pkmers = kmers[node];
            if (pkmers.size() >= SET_CUTOFF) {
                // ++stats.qsearch;
                return kmers_set[node].contains(kmer);
            } else {
                // ++stats.ssearch;
                return std::find(pkmers.begin(), pkmers.end(), kmer) != pkmers.end();
            }
        }

        KmerPerNodeLen add_kmer(NodeLoc node, Kmer kmer) {
            auto &pkmers = kmers[node];
            pkmers.emplace_back(kmer);

            if (pkmers.size() == SET_CUTOFF) {
                // ++stats.nsets;
                kmers_set[node].insert(pkmers.begin(), pkmers.end());
            } else if (pkmers.size() > SET_CUTOFF) {
                kmers_set[node].insert(kmer);
            }
            return pkmers.size();
        }

        KmerPerNodeLen num_kmers(NodeLoc node) const {
            return kmers[node].size();
        }
    } kd;

    void _bfs() {
        while (!q.empty()) {
            auto nid = q.top(); q.pop(); in_q[nid] = false;
            // std::cerr << "popping " << nid << std::endl;
            const auto &node = graph.node(nid);
            auto &done_idx = kd.done_idx[nid];

            LetterLoc loc = lloc.compress(NodePos(nid, 0));

            if (node.seg.size() >= Kmer::K) {
                // std::cerr << "case 1" << std::endl;
                Kmer kmer;
                for (; done_idx < kd.kmers[nid].size(); ++done_idx) {
                    kmer = kd.kmers[nid][done_idx];
                    if (kmer.is_complete())
                        pairs.emplace_back(kmer, loc);
                    _walk_node(kmer, node.seg, loc, 1, Kmer::K);
                    // for (NodeLen i = 1; i < Kmer::K; ++i) {
                    //     kmer.push_back(node.seg[i-1]);
                    //     pairs.emplace_back(kmer, loc + i);
                    // }
                }
                _walk_node(kmer, node.seg, loc, Kmer::K, node.seg.size());
                // for (NodeLen i = Kmer::K; i < node.seg.size(); ++i) {
                //     kmer.push_back(node.seg[i-1]);
                //     pairs.emplace_back(kmer, loc + i);
                // }
                kmer.push_back(node.seg.back());
                _push_neighbours(kmer, nid);
                // for (const auto &fwd : graph.forward_from(nid)) {
                //     if (!kd.exists(fwd.node_id, kmer)) {
                //         kd.add_kmer(fwd.node_id, kmer);
                //         if (!in_q[fwd.node_id]) {
                //             in_q[fwd.node_id] = true;
                //             q.push(fwd.node_id);
                //         }
                //     }
                // }
            } else {
                // std::cerr << "case 2" << std::endl;
                // many starting, many ending
                for (; done_idx < kd.kmers[nid].size(); ++done_idx) {
                    Kmer kmer = kd.kmers[nid][done_idx];
                    if (kmer.is_complete())
                        pairs.emplace_back(kmer, loc);
                    _walk_node(kmer, node.seg, loc, 1, node.seg.size());
                    // for (NodeLen i = 1; i < node.seg.size(); ++i) {
                    //     kmer.push_back(node.seg[i-1]);
                    //     pairs.emplace_back(kmer, loc + i);
                    // }
                    kmer.push_back(node.seg.back());
                    _push_neighbours(kmer, nid);
                    // for (const auto &fwd : graph.forward_from(nid)) {
                    //     if (!kd.exists(fwd.node_id, kmer)) {
                    //         kd.add_kmer(fwd.node_id, kmer);
                    //         if (!in_q[fwd.node_id]) {
                    //             in_q[fwd.node_id] = true;
                    //             q.push(fwd.node_id);
                    //         }
                    //     }
                    // }
                }
            }
        }
    }

    void _walk_node(Kmer &kmer, const Str &seg,
            LetterLoc loc, NodeLen start, NodeLen end) {
        // std::cerr << "walk node '" << kmer << "' "
        //     << loc << " " << start << " "
        //     << end << std::endl;

        // pairs.emplace_back(kmer, loc);
        for (NodeLen i = start; i < end; ++i) {
            kmer.push_back(seg[i-1]);
            if (kmer.is_complete())
                pairs.emplace_back(kmer, loc + i);
        }
    }

    void _push_neighbours(Kmer &kmer, NodeLoc nid) {
        // std::cerr << "push neighbour " << kmer << " " << nid << std::endl;
        for (const auto &fwd : graph.forward_from(nid)) {
            if (!kd.exists(fwd.node_id, kmer)) {
                kd.add_kmer(fwd.node_id, kmer);
                if (!in_q[fwd.node_id]) {
                    // std::cerr << " Push " << fwd.node_id << std::endl;
                    in_q[fwd.node_id] = true;
                    q.push(fwd.node_id);
                } else {
                    // std::cerr << " already pushed " << fwd.node_id << std::endl;
                }
            }
        }
    }
};

} /* namespace triegraph */

#endif /* __TRIEGRAPH_BUILDER_NBFS_H__ */