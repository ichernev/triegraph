// SPDX-License-Identifier: MPL-2.0
/*
 * Copyright (c) 2021, Iskren Chernev
 */

#include "triegraph/util/util.h"
#include "triegraph/trie/kmer.h"
#include "triegraph/trie/dkmer.h"
#include "triegraph/alphabet/dna_letter.h"

#include <chrono>

using u32 = triegraph::u32;

template <typename Kmer>
static void test_insertions(u32 ins) {
    int total = 0;
    srand(0);
    for (u32 i = 0; i < ins; ++i) {
        Kmer k = Kmer::empty();
        for (u32 j = 0; j < Kmer::K; ++j) {
            k.push_back(typename Kmer::Letter { rand() % Kmer::Letter::num_options });
        }
        total += k.data;
    }
    std::cerr << "checksum: " << total << std::endl;
}

template <typename Kmer>
static void test_decompress(u32 iter, u32 max_compressed) {
    int total = 0;
    srand(0);
    for (u32 i = 0; i < iter; ++i) {
        Kmer k = Kmer::from_compressed(rand() % max_compressed);
        total += k.data;
    }
    std::cerr << "checksum: " << total << std::endl;
}

template <typename Kmer>
static void test_compress_decompress(u32 iter, u32 max_compressed) {
    int total = 0;
    srand(0);
    for (u32 i = 0; i < iter; ++i) {
        Kmer k = Kmer::from_compressed(rand() % max_compressed);
        total += k.compress();
    }
    std::cerr << "checksum: " << total << std::endl;
}


struct Timer {
    using steady_clock = std::chrono::steady_clock;
    std::string name;
    std::chrono::time_point<steady_clock> start;
    Timer(const std::string &name) : name(name), start(steady_clock::now()) {}
    ~Timer() {
        auto end = steady_clock::now();
        std::cerr << "Timer:" << name << " "
            << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms"
            << std::endl;
    }
};

int main() {
    using Kmer = triegraph::Kmer<triegraph::dna::DnaLetter, u32, 15>;
    using DKmer = triegraph::DKmer<triegraph::dna::DnaLetter, u32>;

    DKmer::set_settings({
        .trie_depth = Kmer::K,
        .on_mask = Kmer::ON_MASK,
    });

    {
        auto t = Timer("dkmer-ins"); test_insertions<DKmer>(10 * 1000 * 1000);
    }
    {
        auto t = Timer("dkmer-decomp"); test_decompress<DKmer>(100 * 1000 * 1000, DKmer::NUM_COMPRESSED);
    }
    {
        auto t = Timer("dkmer-comp-decomp"); test_compress_decompress<DKmer>(100 * 1000 * 1000, DKmer::NUM_COMPRESSED);
    }
    {
        auto t = Timer("kmer-ins"); test_insertions<Kmer>(10 * 1000 * 1000);
    }
    {
        auto t = Timer("kmer-decomp"); test_decompress<Kmer>(100 * 1000 * 1000, Kmer::NUM_COMPRESSED);
    }
    {
        auto t = Timer("kmer-comp-decomp"); test_compress_decompress<Kmer>(100 * 1000 * 1000, Kmer::NUM_COMPRESSED);
    }

    return 0;
}
