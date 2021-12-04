// SPDX-License-Identifier: MPL-2.0
/*
 * Copyright (c) 2021, Iskren Chernev
 */

#include "testlib/dna.h"
#include "testlib/test.h"
#include "testlib/trie/builder/tester.h"

using TG = test::Manager_RK;
int m = test::define_module(__FILE__,
    &test::TrieBuilderTester<TG, TG::TrieBuilderLBFS>::define_tests);
