#ifndef __COMPLEXITY_ESTIMATOR_H__
#define __COMPLEXITY_ESTIMATOR_H__

#include "util/logger.h"

#include <queue>
#include <unordered_map>
#include <vector>

namespace triegraph {

template <typename Graph_, typename TopOrder_, typename KmerHolder_>
struct ComplexityEstimator {
    using Graph = Graph_;
    using TopOrder = TopOrder_;
    using KmerHolder = KmerHolder_;
    using NodeLoc = Graph::NodeLoc;
    using EdgeLoc = Graph::EdgeLoc;

    ComplexityEstimator(
            const Graph &graph,
            const TopOrder &top_ord,
            u32 trie_depth,
            KmerHolder backedge_init,
            u32 backedge_max_trav)
        : graph(graph),
          top_ord(top_ord),
          trie_depth(trie_depth),
          backedge_init(backedge_init),
          backedge_max_trav(backedge_max_trav),
          pq(top_ord.comp())
    {
        o_pwr.resize(trie_depth + 1);
        o_pwr[0] = 1;
        for (u32 i = 1; i < o_pwr.size(); ++i) {
            o_pwr[i] = o_pwr[i-1] * Graph::Str::Letter::num_options;
        }
    }

    ComplexityEstimator &compute() {
        start.resize(graph.num_nodes(), 0);
        end.resize(graph.num_nodes());

        auto scope = Logger::get().begin_scoped("complexity-estimator");

        Logger::get().begin("traverse DAG");
        // 1. Traverse DAG
        for (const auto &nid : top_ord.get_ordered_nodes()) {
            // std::cerr << "nid " << nid << std::endl;
            if (graph.backward_from(nid).empty()) {
                start[nid] = end[nid] = 1;
                // std::cerr << "nid " << nid << " is initial" << std::endl;
                continue;
            }
            bool verbose = false;
            if (graph.node(nid).seg_id == "105677") {
                verbose = true;
                std::cerr << std::endl;
            }
            for (const auto &bwd : graph.backward_from(nid)) {
                if (top_ord.is_backedge(graph.reverse_edge(bwd.edge_id))) {
                    // std::cerr << " got bw edge, incr with " << backedge_init << std::endl;
                    _incr_start(nid, backedge_init);
                    if (verbose) {
                        std::cerr << "NID:" << nid << " "
                            << "seg_id:" << graph.node(nid).seg_id << " "
                            << "BE from " << graph.node(bwd.node_id).seg_id << " "
                            << "BE init " << backedge_init <<std::endl;
                    }
                } else {
                    // std::cerr << " got normal edge from " << bwd.node_id << std::endl;
                    _incr_start(nid, end[bwd.node_id]);
                    if (verbose) {
                        std::cerr << "NID:" << nid << " "
                            << "seg_id:" << graph.node(nid).seg_id << " "
                            << "end from " << graph.node(bwd.node_id).seg_id << " "
                            << "val " << end[bwd.node_id] << std::endl;
                    }
                }
            }
            if (verbose) {
                        std::cerr << "NID:" << nid << " "
                            << "seg_id:" << graph.node(nid).seg_id << " "
                            << "start: " << start[nid] << std::endl;
            }
            // std::cerr << "start " << start[nid] << " end " << _compute_end(nid) << std::endl;
            end[nid] = _compute_end(nid);
        }

        Logger::get().end().begin("seed backedges");
        // 2. Seed backedges
        in_pq.resize(graph.num_nodes(), false);
        for (const auto &edge : graph.forward_edges()) {
            if (top_ord.is_backedge(edge) && end[edge.from] > backedge_init) {
                start[edge.to] += end[edge.from] - backedge_init;
                pushed[edge.edge_id] = end[edge.from];
                trav[edge.edge_id] = 1;

                _add_pq(edge.to);
            }
        }

        Logger::get().end().begin("traverse loops");
        // 3. Push around until done
        while (!pq.empty()) {
            auto nid = pq.top(); pq.pop(); in_pq[nid] = false;

            auto new_end = _compute_end(nid);
            if (new_end == end[nid])
                continue;
            auto incr = new_end - end[nid];
            end[nid] = new_end;
            for (const auto &fwd : graph.forward_from(nid)) {
                if (top_ord.is_backedge(graph.forward_edge(fwd.edge_id))) {
                    auto old_pushed = _get_pushed(fwd.edge_id);
                    if (new_end > old_pushed) {
                        auto &t = trav[fwd.edge_id];
                        ++ t;
                        if (t == backedge_max_trav) {
                            // set "infinity"
                            pushed[fwd.edge_id] = _max_end(nid);
                        } else {
                            pushed[fwd.edge_id] = new_end;
                        }
                        _incr_start(nid, pushed[fwd.edge_id] - old_pushed);
                        _add_pq(fwd.node_id);
                    }
                } else {
                    _incr_start(fwd.node_id, incr);
                    _add_pq(fwd.node_id);
                }
            }
        }

        return *this;
    }

    const std::vector<NodeLoc> &get_ends() const { return end; }
    const std::vector<NodeLoc> &get_starts() const { return start; }

private:

    void _add_pq(NodeLoc nid) {
        if (!in_pq[nid]) {
            in_pq[nid] = true;
            pq.push(nid);
        }
    }

    KmerHolder _get_pushed(EdgeLoc edge_id) const {
        auto it = pushed.find(edge_id);
        if (it != pushed.end())
            return it->second;
        return backedge_init;
    }

    // this prevents overflow, assuming KmerHolder can hold at least twice
    // o_pwr[trie_depth]
    void _incr_start(NodeLoc nid, KmerHolder incr) {
        start[nid] += incr;
        if (start[nid] > o_pwr[trie_depth]) {
            // std::cerr << "hmm, start overflow " << nid << std::endl;
            start[nid] = o_pwr[trie_depth];
        }
    }

    KmerHolder _compute_end(NodeLoc nid) const {
        return std::min(start[nid], _max_end(nid));
    }

    KmerHolder _max_end(NodeLoc nid) const {
        auto l = graph.node(nid).seg.size();
        return l >= trie_depth ? 1 : o_pwr[trie_depth - l];
    }


    const Graph &graph;
    const TopOrder &top_ord;
    u32 trie_depth;
    KmerHolder backedge_init;
    u32 backedge_max_trav;

    std::vector<KmerHolder> o_pwr; // Letter::num_options ^ i up to trie_depth

    std::priority_queue<NodeLoc, std::vector<NodeLoc>, typename TopOrder::Comparator> pq;
    std::vector<bool> in_pq;

    std::vector<KmerHolder> start; // indexed by NodeLoc
    std::vector<KmerHolder> end;   // indexed by NodeLoc
    std::unordered_map<EdgeLoc, u32> trav;          // for backedges
    std::unordered_map<EdgeLoc, KmerHolder> pushed; // for backedges
};


} /* namespace triegraph */

#endif /* __COMPLEXITY_ESTIMATOR_H__ */
