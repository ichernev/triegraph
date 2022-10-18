// SPDX-License-Identifier: MPL-2.0
/*
 * Copyright (c) 2021, Iskren Chernev
 */

#include "triegraph/dna_config.h"
#include "triegraph/manager.h"
#include "testlib/test.h"
#include "triegraph/util/compact_vector.h"
#include "triegraph/util/util.h"
#include "triegraph/util/vector_pairs.h"
#include "triegraph/util/timer.h"

#include <random>

using namespace triegraph;
using triegraph::dna::CfgFlags;

int m = test::define_module(__FILE__, [] {
    test::define_test("wtf", [] {
        using TG_CV = triegraph::Manager<triegraph::dna::DnaConfig<0,
            CfgFlags::VP_DUAL_IMPL | CfgFlags::TD_SORTED_VECTOR | CfgFlags::CV_ELEMS>>;
        auto sort_n = [](u32 n) {
            auto vp = TG_CV::VectorPairs {};
            // triegraph::impl::cvx = &vp.get_v1();
            compact_vector_set_bits(vp.get_v1(), 25);
            compact_vector_set_bits(vp.get_v2(), 25);
            auto gen = std::mt19937 {};

            for (u32 i = 0; i < n; ++i) {
                vp.emplace_back(
                        gen() % ((1 << 25) - 1),
                        gen() % ((1 << 25) - 1));
            }
            // std::cerr << "sorting" << std::endl;
            vp.sort_by_fwd();
            // for (const auto &p : vp.fwd_pairs()) {
            //     std::cerr << p.first << " " << p.second << std::endl;
            // }
        };

        for (u32 n = 100000; n <= 1000000; n += 100000) {
            auto beg = Timer<>::now();
            sort_n(n);
            std::cerr << n << " took " << beg.elapsed() << "ms" << std::endl;
            // std::cerr << "assign = " << triegraph::impl::num_assign << std::endl
            //     << "cmp = " << triegraph::impl::num_cmp << std::endl
            //     << "cmp- = " << triegraph::impl::num_cmp_n << std::endl
            //     << "cmp0 = " << triegraph::impl::num_cmp_z << std::endl
            //     << "cmp+ = " << triegraph::impl::num_cmp_p << std::endl
            //     << "eq = " << triegraph::impl::num_eq << std::endl
            //     << "swap = " << triegraph::impl::num_swap << std::endl;
            // Timer end = Timer::now();
        }
        std::cerr << "sorted!!" << std::endl;
        // auto graph = TG_CV::Graph::from_file("data/pasgal-MHC1.gfa", {});
        // auto pairs = TG_CV::graph_to_pairs<TG_CV::TrieBuilderNBFS>(
        //         graph, triegraph::MapCfg {});
        // pairs.sort_by_fwd().unique();
    });
});
