#ifndef __TRIE_KMER_CODEC_H__
#define __TRIE_KMER_CODEC_H__

namespace triegraph {

template <typename Kmer, typename KmerComp = Kmer::Holder, bool allow_inner = false>
struct KmerCodec {
    using ext_type = Kmer;
    using int_type = KmerComp;

    static int_type to_int(const ext_type &kmer) { return kmer.compress_leaf(); }
    static ext_type to_ext(const int_type &krepr) { return Kmer::from_compressed_leaf(krepr); }
};

template <typename Kmer, typename KmerComp>
struct KmerCodec<Kmer, KmerComp, true> {
    using ext_type = Kmer;
    using int_type = KmerComp;

    static int_type to_int(const ext_type &kmer) { return kmer.compress(); }
    static ext_type to_ext(const int_type &krepr) { return Kmer::from_compressed(krepr); }
};

} /* namespace triegraph */

#endif /* __TRIE_KMER_CODEC_H__ */
