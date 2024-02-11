import os, re, subprocess
expath = "/mnt/ncsudrive/p/pchandr6/work/sem1/ece563/prj1/ece_563_cache_simulation/experiments/"
# Vary L1 cache size from 1 KB to 32 KB in powers of 2 (1, 2,..., 32)
l1Sizes = [2**size for size in range(10,16)]
# For every l1 size, run simulation with varying blocksizes as follows
blocksizes = [16, 32,64, 128]
# Associativity is fixed
assoc = 4
# No L2; No prefetching

for l1Size in l1Sizes:
    newpath = f"{expath}/out/experiment3/l1_{str(l1Size)}/"
    mkdirCmd = f"mkdir -p {newpath} "
    subprocess.check_call(mkdirCmd, shell=True)
    for blocksize in blocksizes:
        cmd = f"make BLOCKSIZE={str(blocksize)} " \
                       f"L1_SIZE={l1Size} L1_ASSOC={str(assoc)} " \
                       f"L2_SIZE=0 L2_ASSOC=0 " \
                       f"PREF_N=0 PREF_M=0 test > {newpath}/blocksize_{str(blocksize)}.txt "
        print(cmd)
        subprocess.check_call(cmd, shell=True)