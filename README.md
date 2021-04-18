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
  - location chooser: for Back Track and Point BFS, choose starting locations
    so that every path of length N has at least one starting location on it.
- Handle: a location in the triegraph:
  - graph (node + position inside the node)
  - OR
  - trie (kmer)
- Edit Edge: a Handle, letter and operation (MATCH, SUB, INS, DEL)
- TrieGraph: contains TrieData and Graph, and allows iterating over forward
  edit edges, and backward handles for every handle


Building the Trie
=================

Building the trie is a process that takes as input a graph, optional starting
points, and depth and produces a list of (kmer, graph-location) pairs.
Additionally the kmers might be all the same length (the depth of the trie),
or some might be shorter.

Back Track
----------

This is the simplest algorithm. Given a starting location it runs a back track
that builds the kmer as it descends and stops when the desired trie depth is
reached.

Properties:
- simple
- slightly slower than rolling bfs, faster than point bfs
- supports selective start locations
- does not support shorter kmers

Done Optimizations:
- traverse the full length of every node instead of letter by letter
  - observed speedup 2x (on hg22)

(Rolling) BFS
-------------

This algorithm is based on the observation, that if there is a kmer that ends
at a particular graph location, adding a new letter to the kmer (and possibly
dropping the oldest one), will produce a kmer that points to the next graph
location.

The algorithm first chooses where to start the bfs from. All connected
components are discovered, and for each component all starting nodes are
selected. If no starting node is present, a random node is selected.

Then the algorithm keeps track of a sequence of kmers that end in each graph
location. Each kmer-location pair can be processed, by adjusting the kmer so
that it points to all neighbouring graph locations. When all kmer-location
pairs are processed the algorithm is done.

Properties:
- fast
- does not support selective start locations
- does not support shorter kmers

Done Optimizations:
- top sort nodes and traverse in top sort order => done in Node BFS

Point BFS
---------

This algorithm is similar to the Back Track, in that it starts from a given
location and accumulates the kmer as it proceeds through the graph. It uses BFS
instead of back track, which lets it observe how many kmer-to-graph pairs are
present on each level (kmer length). This enables the algorithm to stop earlier
and produce shorter-than-trie-depth kmers in order to handle combinatorial
explosions.

Properties:
- slowest
- supports selective start locations
- supports shorter kmers

Done Optimizations:
- special case: kmer fits in starting node
  - 4.5x speedup
- special case: kmer fits in starting node + single successor
- special case: kmer fits in starting node + all successors

Node BFS
--------

This algorithm is designed to give optimal performance on linear graphs, single
pass for DAGs and minimize visited nodes cyclic graphs.

It traverses graph nodes, as opposed to the other algorithms that use letter
locations. Every node keeps kmers that reach it's begining. Also nodes are
traversed using a priority queue on topological order, so if node A can
"provide" kmers for node B, A is traversed before B.

Properties:
- very fast
- does not support selective start locations
- does not support shorter kmers

(Extra) choosing starting locations
-----------------------------------

Both the Back Track and the Point BFS support running the algorithm from
a select number of locations (instead of all locations at once). In practise it
is useful to choose starting locations such that for any given path in the
graph of length N, there is at least one starting location.

To find locations that satisfy this criteria we'll tag all graph locations with
a number from 0 to N-1. If we have two adjacent locations A and B, tag[A]
< tag[B] if tag[A] < N-1. It's easy to proof that each path of length N will
contain at least one location A, such that tag[A] == N-1.

To do the tagging, we compute the inbound degree of each node, give a tag value
of 0 to all nodes and each starting node is given a tag of N-1 (starting nodes
are chosen the same way as in Rolling BFS) and added to the queue. For each
node A popped from the queue we make sure that all neighbour nodes have a tag
at least tag[A] + 1 (unless tag[A] == N-1), and if this is the last inbound
connection we add the neighbour to the queue. If the queue is empty, then we
pick a random node that has been touched, but still has inbound degree > 0,
assign tag = N-1, and traverse it.

(Extra) Complexity estimator
----------------------------

The amount of kmers at the start of every node can be estimated (upper bound)
with a simple algorithm, roughtly following Node BFS, but without the actual
kmer computation.

Each node has a start number (how many different kmers reach the start of the
node) and end number (how many kmers reach the end).

* start[i] = sum(end[j]) for every node j with edge j-i
* end[i] = min(start[i], maxend(L)), where L is the length of node i
* maxend(L) = 1 if L >= K else O^(K-L), where O is number of letter options (4
  for DNA)

These numbers can be computed with one pass in topological order (if we ignore
loops). Loops introduce a problem, because they might reinforce themselves
(demand higher and higher values on each pass). Most hopefully won't, so after
the numbers are ready ignoring loops, put all through the back edges (the
loops), and mark the ones that do reinforce themselves as infinite.

After the numbers are computed, we can try to isolate complexity components
that have high numbers, and run a shorter-kmer-aware algorithm on those.
A complexity component is a connected component ending with nodes with L >= K.

### Complexity Components and Walker

When a high-complexity node is found (via complexity estimator), usually
isolating just this node is not enough due to the fact that the complexity
actually starts before this node. ComplexityComponent stores a subgraph where
all nodes are short (less than K), and lets you extract the component with
ComplexityComponent::Builder (starting from a given node).
ComplexityComponentWalker is a higher level interface that builds the necessary
ComplexityComponents by a bunch of starting (complex) positions, and then
provides 2 iterators for traversing both the "complex" and the "non-complex"
positions.

At the moment just graph-location iterators are provided, so Back Track and
Point BFS can be used.

TODO Veriant/Optimization:
- Support subgraph for non-complex positions so BFS and Node BFS could be used
  (much faster).

Holding the TrieData
====================

TrieData holds the information produced by the TrieGraphBuilder. In theory it
is a list of pairs of kmer and a graph location. In practise there should be
fast access by kmer and graph location.

A straight forward solution is to keep 2 maps, one from kmer to graph location
and one from graph location to kmer. These should be multimaps (multiple values
for the same key). There are 3 interchangeable multimap implementations:

SimpleMultimap
--------------

This is just a wrapper around `unordered_multimap` (anything satisfying the
multimap interface will do). The reason a regular multimap doesn't work is the
addition of `key_itrator` and `value_iterator` which in most cases yiled better
performance than using a normal `pair_iterator`.

HybridMultimap
--------------

Some 3rd party libraries offer more (space) efficient hashmap implementations
but rarely offer multimap interface (just a map). To handle those
HybridMultimap uses a map and a vector, where the map maps to a range in the
vector, and thus supports multiple values per key. This is much more space
efficient than mapping to a vector, so was chosen instead.

DenseMultimap
-------------

For best theoretical performance the trie depth should be chosen such as, on
average every kmer of full length points to one graph location and every graph
location points back to one full length kmer. This means that if we assign
a numeric value of all full-length kmers and all graph locations there will be
two full hash tables (every possible key has a value).

To (ab)use this fact, there is no need to keep the keys, instead keys can be
indices in an array of values. To handle cases where some keys have in fact
multiple values assigned (unavoidable in practise), the values are stored in an
array `values[]`, and there is an additional array `start[]`, indexed by key,
which points to the index in the value array where the values for that key
start. So the values for key k are `[values[start[k]], ...,
values[start[k+1]-1]]` (it might be empty list if start[k] == start[k+1]).

So the multimap is represented by two arrays. One of length max-key, and the
other of length num-pairs.

TODO Optimizations:
- implement bit-compact vector, that stores N-bit integers so no space is
  wasted (so 34bit integer will take 34bits, i.e a million integers will take
  4.25G instead of 8G if stored as 64bits).

Done Optimizations:
- SortedVector for `start[]` array

(Extra) SortedVector
--------------------

The `start` vector is sorted, and we can (ab)use this fact to reduce the memory
requirements, by noting that every element is normally just slightly bigger (or
the same) than the previous element.

Storing every N-th element in a full sized integer (called beacons), and all
intermediary diffs in a smaller integer (like char or short), will dramatically
reduce memory footprint. In case the diff doesn't fit in the short integer,
a special sentinel value is used and the actual value is stored in a regular
map.

SortedVector could be used as replacement of the regular vector `start` in
DenseMultimap.

TODO Variants/Optimizations:
- currently the diff is element wise, but it could be switched to beacon-wise
  (full-sized element to current element). The drawback is that the overflow
  map will be consulted more often, but the lookup will take just 2 operations
  (instead of N in the worst case, where N is the distance between beacons).
