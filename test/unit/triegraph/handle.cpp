// SPDX-License-Identifier: MPL-2.0
/*
 * Copyright (c) 2021, Iskren Chernev
 */

#include "triegraph/dna_config.h"
#include "triegraph/manager.h"

#include "testlib/test.h"

int m = test::define_module(__FILE__, [] {

test::define_test("handle32", [] {
    using TG = triegraph::Manager<triegraph::dna::DnaConfig<0>>;
    TG::kmer_set_depth(4);

    auto h0 = TG::Handle();
    assert(!h0.is_valid());

    auto h1 = TG::Handle(TG::Kmer::from_str("acgt"));
    assert( h1.is_trie());
    assert(!h1.is_graph());
    assert( h1.is_valid());

    auto h2 = TG::Handle(0, 0);
    assert(!h2.is_trie());
    assert( h2.is_graph());
    assert( h2.is_valid());
});

test::define_test("handle64", [] {
    using TG = triegraph::Manager<
        triegraph::dna::DnaConfig<0, triegraph::dna::CfgFlags::WEB_SCALE>>;
    TG::kmer_set_depth(4);

    auto h0 = TG::Handle();
    assert(!h0.is_valid());

    auto h1 = TG::Handle(TG::Kmer::from_str("acgt"));
    assert( h1.is_trie());
    assert(!h1.is_graph());
    assert( h1.is_valid());

    auto h2 = TG::Handle(0, 0);
    assert(!h2.is_trie());
    assert( h2.is_graph());
    assert( h2.is_valid());
});

});
