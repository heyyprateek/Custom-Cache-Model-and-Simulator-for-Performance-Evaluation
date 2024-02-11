CC = g++
OPT = -O3
#OPT = -g
WARN = -Wall
STD = -std=c++11
CFLAGS = $(OPT) $(STD) $(WARN) $(INC) $(LIB)

# List all your .cc/.cpp files here (source files, excluding header files)
SIM_SRC = src/sim.cc

# List corresponding compiled object files here (.o files)
SIM_OBJ = src/sim.o
 
#################################

# default rule

all: sim
	@echo "my work is done here..."


# rule for making sim

sim: $(SIM_OBJ)
	$(CC) -o sim $(CFLAGS) $(SIM_OBJ) -lm
	@echo "-----------DONE WITH sim-----------"


# generic rule for converting any .cc file to any .o file
 
.cc.o:
	$(CC) $(CFLAGS) -c $*.cc -o $(SIM_OBJ)

# generic rule for converting any .cpp file to any .o file

.cpp.o:
	$(CC) $(CFLAGS) -c $*.cpp


# type "make clean" to remove all .o files plus the sim binary

clean:
	rm -f src/*.o sim


# type "make clobber" to remove all .o files (leaves sim binary)

clobber:
	rm -f src/*.o

BLOCKSIZE?=16
L1_SIZE?=1024
L1_ASSOC?=1
L2_SIZE?=0
L2_ASSOC?=0
PREF_N?=0
PREF_M?=0
trace_file?=spec/example_trace.txt

test:
	./sim $(BLOCKSIZE) \
		  $(L1_SIZE) $(L1_ASSOC) \
		  $(L2_SIZE) $(L2_ASSOC) \
		  $(PREF_N) $(PREF_M) \
		  $(trace_file)

test_quiz2:
	$(MAKE) L1_SIZE=256 L1_ASSOC=1 BLOCKSIZE=16 trace_file=spec/tagindexBO.txt test

test_quiz3:
	$(MAKE) L1_SIZE=256 L1_ASSOC=2 BLOCKSIZE=16 trace_file=spec/quiz3_trace.txt test

test_quiz4:
	$(MAKE) BLOCKSIZE=16 \
	L1_SIZE=512 L1_ASSOC=2  \
	L2_SIZE=4096 L2_ASSOC=1 \
	trace_file=spec/quiz4_trace.txt test

allvalrun: \
	valrun1 \
	valrun2 \
	valrun3 \
	valrun4 \
	valrun5 \
	valrun6 \
	valrun7 \
	valrun8

valrun1: clean all
	rm -rf out/$@.txt
	$(MAKE) BLOCKSIZE=16 \
	L1_SIZE=1024 L1_ASSOC=1  \
	L2_SIZE=0 L2_ASSOC=0 \
	PREF_N=0 PREF_M=0 \
	trace_file=spec/traces/gcc_trace.txt test \
	> out/$@.txt
	diff -iw val-proj1/val1.16_1024_1_0_0_0_0_gcc.txt out/$@.txt

valrun2: clean all
	rm -rf out/$@.txt
	$(MAKE) BLOCKSIZE=32 \
	L1_SIZE=1024 L1_ASSOC=2  \
	L2_SIZE=0 L2_ASSOC=0 \
	PREF_N=0 PREF_M=0 \
	trace_file=spec/traces/gcc_trace.txt test \
	> out/$@.txt
	diff -iw val-proj1/val2.32_1024_2_0_0_0_0_gcc.txt out/$@.txt

valrun3: clean all
	rm -rf out/$@.txt
	$(MAKE) BLOCKSIZE=16 \
	L1_SIZE=1024 L1_ASSOC=1  \
	L2_SIZE=8192 L2_ASSOC=4 \
	PREF_N=0 PREF_M=0 \
	trace_file=spec/traces/gcc_trace.txt test \
	> out/$@.txt
	diff -iw val-proj1/val3.16_1024_1_8192_4_0_0_gcc.txt out/$@.txt

valrun4: clean all
	rm -rf out/$@.txt
	$(MAKE) BLOCKSIZE=32 \
	L1_SIZE=1024 L1_ASSOC=2  \
	L2_SIZE=12288 L2_ASSOC=6 \
	PREF_N=0 PREF_M=0 \
	trace_file=spec/traces/gcc_trace.txt test \
	> out/$@.txt
	diff -iw val-proj1/val4.32_1024_2_12288_6_0_0_gcc.txt out/$@.txt

valrun5: clean all
	rm -rf out/$@.txt
	$(MAKE) BLOCKSIZE=16 \
	L1_SIZE=1024 L1_ASSOC=1  \
	L2_SIZE=0 L2_ASSOC=0 \
	PREF_N=1 PREF_M=4 \
	trace_file=spec/traces/gcc_trace.txt test \
	> out/$@.txt
	diff -iw val-proj1/val5.16_1024_1_0_0_1_4_gcc.txt out/$@.txt

valrun6: clean all
	rm -rf out/$@.txt
	$(MAKE) BLOCKSIZE=32 \
	L1_SIZE=1024 L1_ASSOC=2  \
	L2_SIZE=0 L2_ASSOC=0 \
	PREF_N=3 PREF_M=1 \
	trace_file=spec/traces/gcc_trace.txt test \
	> out/$@.txt
	# diff -iw val-proj1/val6.32_1024_2_0_0_3_1_gcc.txt out/$@.txt

valrun7: clean all
	rm -rf out/$@.txt
	$(MAKE) BLOCKSIZE=16 \
	L1_SIZE=1024 L1_ASSOC=1  \
	L2_SIZE=8192 L2_ASSOC=4 \
	PREF_N=3 PREF_M=4 \
	trace_file=spec/traces/gcc_trace.txt test \
	> out/$@.txt
	diff -iw val-proj1/val7.16_1024_1_8192_4_3_4_gcc.txt out/$@.txt

valrun8: clean all
	rm -rf out/$@.txt
	$(MAKE) BLOCKSIZE=32 \
	L1_SIZE=1024 L1_ASSOC=2  \
	L2_SIZE=12288 L2_ASSOC=6 \
	PREF_N=7 PREF_M=6 \
	trace_file=spec/traces/gcc_trace.txt test \
	> out/$@.txt
	diff -iw val-proj1/val8.32_1024_2_12288_6_7_6_gcc.txt out/$@.txt

exrun1:
	rm -rf out/$@.txt
	$(MAKE) BLOCKSIZE=64 \
	L1_SIZE=3584 L1_ASSOC=7  \
	L2_SIZE=0 L2_ASSOC=0 \
	PREF_N=0 PREF_M=0 \
	trace_file=spec/traces/gcc_trace.txt test \
	> out/$@.txt
	diff -iw extra_runs/extra1.64_3584_7_0_0_0_0_gcc.txt out/$@.txt

exrun2:
	rm -rf out/$@.txt
	$(MAKE) BLOCKSIZE=16 \
	L1_SIZE=1024 L1_ASSOC=64  \
	L2_SIZE=0 L2_ASSOC=0 \
	PREF_N=0 PREF_M=0 \
	trace_file=spec/traces/gcc_trace.txt test \
	> out/$@.txt
	diff -iw extra_runs/extra2.16_1024_64_0_0_0_0_gcc.txt out/$@.txt

exrun3:
	rm -rf out/$@.txt
	$(MAKE) BLOCKSIZE=16 \
	L1_SIZE=1024 L1_ASSOC=2  \
	L2_SIZE=0 L2_ASSOC=0 \
	PREF_N=0 PREF_M=0 \
	trace_file=spec/traces/perl_trace.txt test \
	> out/$@.txt
	diff -iw extra_runs/extra3.16_1024_2_0_0_0_0_perl.txt out/$@.txt

exrun4:
	rm -rf out/$@.txt
	$(MAKE) BLOCKSIZE=32 \
	L1_SIZE=1024 L1_ASSOC=2  \
	L2_SIZE=8192 L2_ASSOC=4 \
	PREF_N=0 PREF_M=0 \
	trace_file=spec/traces/vortex_trace.txt test \
	> out/$@.txt
	diff -iw extra_runs/extra4.32_1024_2_8192_4_0_0_vortex.txt out/$@.txt

exrun5:
	rm -rf out/$@.txt
	$(MAKE) BLOCKSIZE=64 \
	L1_SIZE=8192 L1_ASSOC=4  \
	L2_SIZE=0 L2_ASSOC=0 \
	PREF_N=8 PREF_M=4 \
	trace_file=spec/traces/compress_trace.txt test \
	> out/$@.txt
	diff -iw extra_runs/extra5.64_8192_4_0_0_8_4_compress.txt out/$@.txt