#include "testlib/test.h"
#include "testlib/trie/builder/tester.h"

#include "dna_config.h"
#include "manager.h"

using namespace triegraph;

using TG = Manager<dna::DnaConfig<0, false, true>>;
int m = test::define_module(__FILE__,
    &test::TrieBuilderTester<TG, TG::TrieBuilderLBFS>::define_tests);
