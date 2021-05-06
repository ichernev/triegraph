#include "testlib/dna.h"
#include "testlib/test.h"
#include "testlib/trie/builder/tester.h"

using TG = test::Manager_RK;
int m = test::define_module(__FILE__,
    &test::TrieBuilderTester<TG, TG::TrieBuilderBT>::define_tests);
