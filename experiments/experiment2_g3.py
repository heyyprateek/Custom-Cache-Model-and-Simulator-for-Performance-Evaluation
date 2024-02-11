import os, re, subprocess
l1Sizes = [2**size for size in range(10,15)]
assocs = [1, 2, 4, 8, "fully"]
expath = "/mnt/ncsudrive/p/pchandr6/work/sem1/ece563/prj1/ece_563_cache_simulation/experiments/"
blocksize = 32
for assoc in assocs:
    newpath = f"{expath}/out/experiment2/{str(assoc)}/"
    mkdirCmd = f"mkdir -p {newpath} "
    subprocess.check_call(mkdirCmd, shell=True)
    for size in l1Sizes:
        if assoc == "fully":
            assoc = int(size/blocksize)
        cmd = f"make BLOCKSIZE={str(blocksize)} " \
                       f"L1_SIZE={size} L1_ASSOC={str(assoc)} " \
                       f"L2_SIZE=16384 L2_ASSOC=8 " \
                       f"PREF_N=0 PREF_M=0 test > {newpath}/content_{str(size)}.txt "
        print(cmd)
        subprocess.check_call(cmd, shell=True)
        subprocess.check_call(f"grep -inr 'L1 miss rate' {newpath}/*.txt", shell=True)
        subprocess.check_call(f"grep -inr 'L2 miss rate' {newpath}/*.txt", shell=True)