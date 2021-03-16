import sys

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

def human(n):
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

    hcoef = {1: '', 1000: 'k', 1000000: 'm', 1000000000: 'b'}
    b = hcoef[coef]
    if n >= 100:
        return f'{n}{b}'
    elif n >= 10:
        a = trim(f'{orig_n/coef:.1f}')
        return f'{a}{b}'
    else:
        a = trim(f'{orig_n/coef:.2f}')
        return f'{a}{b}'

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

def main(args):
    n = parse_human(args[0])
    ff = 2.0 if len(args) == 1 else float(args[1])
    print(f'n           {n}')
    print(f'ff          {ff:.1f}')
    k = log4(n)
    print(f'k = log4(n) {k}')
    nkmers = 4 ** k
    print(f'num kmers   {human(4 ** k)}')
    npairs = int(ff * n)
    print(f'num pairs   {human(npairs)}')

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
    main(sys.argv[1:])
