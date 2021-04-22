import sys
import argparse
# from collections import OrderedDict

OrderedDict = dict

def log4(n):
    if n == 1:
        return 0
    elif n < 4:
        return 1
    return 1 + log4((n + 3)//4)

def log2(n):
    if n == 1:
        return 0
    return 1 + log2((n+1)//2)

def human(n, hcoef):
    orig_n = n
    coef = 1
    while not(n < 1000):
        n //= 1000
        coef *= 1000

    def trim(x):
        while x[-1] == '0':
            x = x[0:-1]
        if x[-1] == '.':
            x = x[0:-1]
        return x

    # hcoef = {1: '', 1000: 'k', 1000000: 'm', 1000000000: 'b'}
    b = hcoef[coef]
    if n >= 100:
        return f'{n}{b}'
    elif n >= 10:
        a = trim(f'{orig_n/coef:.1f}')
        return f'{a}{b}'
    else:
        a = trim(f'{orig_n/coef:.2f}')
        return f'{a}{b}'

def human_mem(n):
    return human(n, {1: 'b', 1000: 'kb', 1000000: 'mb', 1000000000: 'gb'})

def human_mem_bits(n):
    return human_mem(n // 8)

def human_cnt(n):
    return human(n, {1: '', 1000: 'k', 1000000: 'm', 1000000000: 'b'})


def parse_human(quant):
    if quant.endswith('b') or quant.endswith('B'):
        return int(float(quant[0:-1]) * 10**9)
    if quant.endswith('m') or quant.endswith('M'):
        return int(float(quant[0:-1]) * 10**6)
    if quant.endswith('k') or quant.endswith('K'):
        return int(float(quant[0:-1]) * 10**3)
    return int(quant)

def xbytes(bits):
    if bits <= 8:
        return 1
    elif bits <= 16:
        return 2
    elif bits <= 32:
        return 4
    elif bits <= 64:
        return 8
    raise Exception("WTF MAN")


class Vec:
    def __init__(self, elems, bits=None):
        self.elems = elems
        self.bits = bits or xbytes(log2(elems))

    def size(self):
        return self.elems * self.bits // 8

    def detail(self):
        return OrderedDict(
            name=self.__class__.__name__,
            size=human_mem_bits(self.elems * self.bits),
            elems=human_cnt(self.elems),
            bits=self.bits,
        )

class SortedVec:
    def __init__(self, elems, bits=None, beacon_every=16, diff_bits=8, overflow_coef=0.05):
        self.elems = elems
        self.bits = bits or xbytes(log2(elems))
        self.beacon_every = beacon_every
        self.diff_bits = diff_bits

        self.num_beacons = self.elems // self.beacon_every
        self.beacon_bits = self.num_beacons * self.bits
        self.diff_bits = self.elems * self.diff_bits
        self.overflow_coef = overflow_coef

    def size(self):
        return int((self.beacon_bits + self.diff_bits) *
                   (1 + self.overflow_coef)) // 8

    def detail(self):
        return OrderedDict(
            name=self.__class__.__name__,
            size=human_mem(self.size()),
            elems=human_cnt(self.elems),
            num_beacons=human_cnt(self.num_beacons),
            beacon_mem=human_mem_bits(self.beacon_bits),
            diff_mem=human_mem_bits(self.diff_bits))

class DMMap:
    def __init__(self, keys, vals, bits=None, sorted_vector=True):
        self.keys = keys
        self.vals = vals
        self.bits = bits or xbytes(max(self.keys, self.vals))
        self.sorted_vector = sorted_vector

        self.vec_class = SortedVec if self.sorted_vector else Vec
        self.begin = self.vec_class(self.keys, self.bits)
        self.elems = Vec(self.vals, self.bits)

    def size(self):
        return self.begin.size() + self.elems.size()

    def detail(self):
        return OrderedDict(
            name=self.__class__.__name__,
            size=human_mem(self.size()),
            begin=self.begin.detail(),
            elems=self.elems.detail(),
        )

class TieredBitset:
    def __init__(self, kmers, allow_inner=False):
        if allow_inner == True:
            self.kmers = kmers
        else:
            # rough estimate on number of kmers (1 + (1/4)^n...)
            self.kmers = kmers + kmers // 4 + kmers // 16

    def size(self):
        return self.kmers // 8

    def detail(self):
        return OrderedDict(
            name=self.__class__.__name__,
            size=human_mem(self.size()),
            elems=human_cnt(self.kmers),
        )

class Pairs:
    def __init__(self, pairs, bits=None):
        self.pairs = pairs
        self.bits = bits or log2(pairs)

    def size(self):
        return self.bits * 2 * self.pairs // 8

    def detail(self):
        return OrderedDict(
            name=self.__class__.__name__,
            size=human_mem(self.size()),
            pairs=human_cnt(self.pairs),
            bits=self.bits)

class TrieData:
    def __init__(self, kmers, locs, pairs, bits=None, allow_inner=False):
        self.kmers = kmers
        self.locs = locs
        self.pairs = pairs
        self.bits = bits or log2(max(self.kmers, self.locs, self.pairs))

        self.t2g = DMMap(self.kmers, self.pairs, self.bits)
        self.g2t = DMMap(self.locs, self.pairs, self.bits)
        self.trie = TieredBitset(self.kmers, allow_inner)

    def size(self):
        return self.t2g.size() + self.g2t.size() + self.trie.size()

    def detail(self):
        return OrderedDict(
            name=self.__class__.__name__,
            size=human_mem(self.size()),
            kmers=human_cnt(self.kmers),
            locs=human_cnt(self.locs),
            pairs=human_cnt(self.pairs),
            t2g=self.t2g.detail(),
            g2t=self.g2t.detail(),
            trie=self.trie.detail(),
        )


def parse_args(args):
    p = argparse.ArgumentParser(description='Display TrieGraph size estimates')
    p.add_argument('-n', '--graph-locations', type=parse_human, required=True)
    p.add_argument('--reverse-complement', action=argparse.BooleanOptionalAction, default=True)
    p.add_argument('--ff', type=float, default=2.0, required=False)
    p.add_argument('--rel-trie-depth', type=int, default=0, required=False)
    p.add_argument('--allow-inner', action='store_true')
    # TODO: Add support for reading any option stored in JSON config, then pass
    # relevant part of config down the DataStructure
    # p.add_argument('--config', type=str, required=False)


    return p.parse_args(args)


def mainx(args):
    opts = parse_args(args)
    n = opts.graph_locations * (2 if opts.reverse_complement else 1)
    ff = opts.ff
    # print(opts)

    # print(f'n           {n}')
    # print(f'ff          {ff:.1f}')
    npairs = int(ff * n)
    # print(f'num pairs   {human_cnt(npairs)}')
    k = log4(npairs) + opts.rel_trie_depth
    # print(f'k = log4(npairs) {k}')
    # print(f'num kmers   {human_cnt(4 ** k)}')

    if opts.allow_inner:
        nkmers = sum([4 ** k for k in range(1, k+1)])
    else:
        nkmers = 4 ** k

    bits = log2(max(nkmers, n, npairs))

    pairs = Pairs(npairs, bits)
    td = TrieData(nkmers, n, npairs, bits, opts.allow_inner)

    import pprint
    pp = pprint.PrettyPrinter(indent=4, width=100, sort_dicts=False, compact=True)
    pp.pprint(pairs.detail())
    pp.pprint(td.detail())

def main(args):
    n = parse_human(args[0])
    ff = 2.0 if len(args) < 2 else float(args[1])
    # kx = -1 if len(args)  < 3 else int(args[2])
    print(f'n           {n}')
    print(f'ff          {ff:.1f}')
    npairs = int(ff * n)
    print(f'num pairs   {human(npairs)}')
    k = log4(npairs)
    print(f'k = log4(npairs) {k}')
    nkmers = 4 ** k
    print(f'num kmers   {human(4 ** k)}')

    bits_per_kmer = k * 2 + 1
    bits_per_loc =  log2(n)
    bits_per_pair = bits_per_kmer + bits_per_loc
    print(f'X raw size  {human(bits_per_pair * npairs // 8)}')

    bytes_per_kmer = xbytes(bits_per_kmer)
    bytes_per_loc = xbytes(bits_per_loc)
    bytes_per_pair = bytes_per_kmer + bytes_per_loc
    print(f'N raw size  {human(bytes_per_pair * npairs)}')
    print()

    bits_per_npairs = log2(npairs)
    bytes_per_npairs = xbytes(bits_per_npairs)
    print(f'X kmer_start  {human(bits_per_npairs * nkmers // 8)}  {bits_per_npairs} * {human(nkmers)}')
    print(f'X kmer_locs   {human(bits_per_loc * npairs // 8)}  {bits_per_loc} * {human(npairs)}')
    print(f'X kmer_locs_s {human(npairs // 8)}')
    total_bit_kmer_to_loc = (bits_per_npairs * nkmers // 8) + (bits_per_loc * npairs // 8) + (npairs // 8)

    print(f'X loc_start   {human(bits_per_npairs * n // 8)}  {bits_per_npairs} * {human(n)}')
    print(f'X loc_kmers   {human(bits_per_kmer * npairs // 8)}  {bits_per_kmer} * {human(npairs)}')
    print(f'X loc_kmers_s {human(npairs // 8)}')
    total_bit_loc_to_kmer = (bits_per_npairs * n // 8) + (bits_per_kmer * npairs // 8) + (npairs // 8)

    print(f'X total       {human(total_bit_kmer_to_loc + total_bit_loc_to_kmer)}  {human(total_bit_kmer_to_loc)} + {human(total_bit_loc_to_kmer)}')
    print()

    print(f'N kmer_start  {human(bytes_per_npairs * nkmers)}  {bytes_per_npairs} * {human(nkmers)}')
    print(f'N kmer_locs   {human(bytes_per_loc * npairs)}  {bytes_per_loc} * {human(npairs)}')
    print(f'N kmer_locs_s {human(npairs // 8)}')
    total_kmer_to_loc = (bytes_per_npairs * nkmers) + (bytes_per_loc * npairs) + (npairs // 8)

    print(f'N loc_start   {human(bytes_per_npairs * n)}  {bytes_per_npairs} * {human(n)}')
    print(f'N loc_kmers   {human(bytes_per_kmer * npairs)}  {bytes_per_kmer} * {human(npairs)}')
    print(f'N loc_kmers_s {human(npairs // 8)}')
    total_loc_to_kmer = (bytes_per_npairs * n) + (bytes_per_kmer * npairs) + (npairs // 8)

    print(f'N total       {human(total_kmer_to_loc + total_loc_to_kmer)}  {human(total_kmer_to_loc)} + {human(total_loc_to_kmer)}')


if __name__ == '__main__':
    mainx(sys.argv[1:])
