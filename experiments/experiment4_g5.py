import os, re, subprocess
expath = "/mnt/ncsudrive/p/pchandr6/work/sem1/ece563/prj1/ece_563_cache_simulation/experiments/"

# L1 cache can assume 4 different sizes 1, 2, 4, 8 KB
l1Sizes = [2**size for size in range(10,14)]

# L2 cache can assume 3 different sizes 16, 32, 64 KB
l2Sizes = [2**size for size in range(14,17)]

# L1_ASSOC=4; L2_ASSOC=8

blocksize = 32
for l2Size in l2Sizes:
    newpath = f"{expath}/out/experiment4/l2_{str(l2Size)}/"
    mkdirCmd = f"mkdir -p {newpath} "
    subprocess.check_call(mkdirCmd, shell=True)
    for l1Size in l1Sizes:
        cmd = f"make BLOCKSIZE={str(blocksize)} " \
                       f"L1_SIZE={l1Size} L1_ASSOC=4 " \
                       f"L2_SIZE={l2Size} L2_ASSOC=8 " \
                       f"PREF_N=0 PREF_M=0 test > {newpath}/content_{str(l1Size)}.txt "
        print(cmd)
        subprocess.check_call(cmd, shell=True)