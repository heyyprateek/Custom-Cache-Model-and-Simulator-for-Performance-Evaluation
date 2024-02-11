import os, re, subprocess
expath = "/mnt/ncsudrive/p/pchandr6/work/sem1/ece563/prj1/ece_563_cache_simulation/experiments/"

# Fixed L1 cache size 1 KB; L2 none
l1Size = 2**10

# Vary stream buffer configuration (PREF_N, PREF_M)
prefetchesBuffers = [(0,0), (1,4), (2,4), (3,4), (4,4)]

# Use Microbenchmark stream_trace.txt
traceFilePath = "/mnt/ncsudrive/p/pchandr6/work/sem1/ece563/prj1/ece_563_cache_simulation/spec/microbenchmark/streams_trace.txt"

# Fixed blocksize 16
blocksize = 16

# Fixed associativity --> direct mapped

newpath = f"{expath}/out/experiment5/"
mkdirCmd = f"mkdir -p {newpath} "
subprocess.check_call(mkdirCmd, shell=True)
for pref_n, pref_m in prefetchesBuffers:
    cmd = f"make BLOCKSIZE={str(blocksize)} " \
                    f"L1_SIZE={l1Size} L1_ASSOC=1 " \
                    f"L2_SIZE=0 L2_ASSOC=0 " \
                    f"PREF_N={str(pref_n)} PREF_M={str(pref_m)} trace_file={traceFilePath} test " \
                    f" > {newpath}/n{str(pref_n)}m{str(pref_m)}.txt "
    print(cmd)
    subprocess.check_call(cmd, shell=True)