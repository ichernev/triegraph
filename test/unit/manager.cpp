#include "manager.h"
#include "util/util.h"
#include "alphabet/dna_letter.h"

// #include <iostream>
#include "testlib/test.h"

using namespace triegraph;

struct Cfg {
    using Letter = dna::DnaLetter;
    using StrHolder = u32;
    using NodeLoc = u32;
    using NodeLen = u32;
    using EdgeLoc = u32;
    using LetterLoc = u32;
    using KmerHolder = u64;
    static constexpr bool triedata_allow_inner = false;
    static constexpr int LetterLocIdxShift = -1;
    static constexpr u32 TDMapType = 0u;
    static constexpr u64 KmerLen = 0;
    static constexpr KmerHolder on_mask = KmerHolder(1) << 63;
};

// template<typename, typename = void>
// constexpr bool is_type_complete_v = false;

// template<typename T>
// constexpr bool is_type_complete_v
//     <T, std::void_t<decltype(sizeof(T))>> = true;

int m = test::define_module(__FILE__, [] {

test::define_test("compiles", [] {
    assert(sizeof(triegraph::Manager<Cfg>::Kmer) == 8);
});

});

// int main() {
//     std::cerr << sizeof(triegraph::Manager<Cfg>::NodeLen) << std::endl;
//     std::cerr << sizeof(triegraph::Manager<Cfg>::Kmer) << std::endl;
//     // std::cerr << is_type_complete_v<Cfg::huu> << std::endl;
//     return 0;
// }