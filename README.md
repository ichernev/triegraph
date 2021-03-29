About
=====

TrieGraph is a data structure designed for searching in string graphs (so far
it is optimized for DNA graphs, but any sequence of characters is supported in
theory). It is basically a trie that points to various locations in a graph, so
that searching can start at the root of the trie, instead of everywhere in the
graph. The Trie holds kmers (a sequence of K or less letters).

Components
==========

- Graph: holds nodes containing character sequences and directed edges
  connecting those nodes. Supports:
  - reading from rgfa file
  - adding reverse complement
  - adding end extensions -- a single letter at the end of all final nodes
    (that is more of an optimization used by the other components)
- TrieData, contains
  - trie2graph: a structure that keeps kmer-to-graph mapping
  - graph2trie: a structure that keeps graph-to-kmer mapping (the reverse of
    trie2graph)
  - active\_trie: a structure that keeps what kmers are present in trie2graph,
    including prefixes (i.e if ACGT maps to location 15 in trie2graph, then A,
    AC, and ACG are "present" according to active\_trie, but not according to
    trie2graph).
- TrieGraphBuilder: an algorithm used to build TrieData, options are
  - BFS: builds the set of kmer-to-graph mapping for the entire graph
  - Back Track: builds the set of kmer-to-graph using a back-track that can
    start from arbitrary graph locations
  - Point BFS: builds the set of kmer-to-graph starting from arbitrary
    locations using a BFS, that can also auto-limit itself to reduce
    combinatorial explosion
- Handle: a location in the triegraph:
  - graph (node + position inside the node)
  - OR
  - trie (kmer)
- Edit Edge: a Handle, letter and operation (MATCH, SUB, INS, DEL)
- TrieGraph: contains TrieData and Graph, and allows iterating over forward
  edit edges, and backward handles


Building the Trie
=================




Holding the TrieData
====================
