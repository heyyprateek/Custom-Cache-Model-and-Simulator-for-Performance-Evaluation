#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <typeinfo>
#include "sim.h"
#include "cache.cpp"

/*  "argc" holds the number of command-line arguments.
    "argv[]" holds the arguments themselves.

    Example:
    ./sim 32 8192 4 262144 8 3 10 gcc_trace.txt
    argc = 9
    argv[0] = "./sim"
    argv[1] = "32"
    argv[2] = "8192"
    ... and so on
*/
int main (int argc, char *argv[]) {
   FILE *fp;			// File pointer.
   char *trace_file;		// This variable holds the trace file name.
   cache_params_t params;	// Look at the sim.h header file for the definition of struct cache_params_t.
   char rw;			// This variable holds the request's type (read or write) obtained from the trace.
   uint32_t addr;		// This variable holds the request's address obtained from the trace.
				// The header file <inttypes.h> above defines signed and unsigned integers of various sizes in a machine-agnostic way.  "uint32_t" is an unsigned integer of 32 bits.

   // Exit with an error if the number of command-line arguments is incorrect.
   if (argc != 9) {
      printf("Error: Expected 8 command-line arguments but was provided %d.\n", (argc - 1));
      exit(EXIT_FAILURE);
   }
    
   // "atoi()" (included by <stdlib.h>) converts a string (char *) to an integer (int).
   params.BLOCKSIZE = (uint32_t) atoi(argv[1]);
   params.L1_SIZE   = (uint32_t) atoi(argv[2]);
   params.L1_ASSOC  = (uint32_t) atoi(argv[3]);
   params.L2_SIZE   = (uint32_t) atoi(argv[4]);
   params.L2_ASSOC  = (uint32_t) atoi(argv[5]);
   params.PREF_N    = (uint32_t) atoi(argv[6]);
   params.PREF_M    = (uint32_t) atoi(argv[7]);
   trace_file       = argv[8];

   // Open the trace file for reading.
   fp = fopen(trace_file, "r");
   if (fp == (FILE *) NULL) {
      // Exit with an error if file open failed.
      printf("Error: Unable to open file %s\n", trace_file);
      exit(EXIT_FAILURE);
   }
    
   // Print simulator configuration.
   printf("===== Simulator configuration =====\n");
   printf("BLOCKSIZE:  %u\n", params.BLOCKSIZE);
   printf("L1_SIZE:    %u\n", params.L1_SIZE);
   printf("L1_ASSOC:   %u\n", params.L1_ASSOC);
   printf("L2_SIZE:    %u\n", params.L2_SIZE);
   printf("L2_ASSOC:   %u\n", params.L2_ASSOC);
   printf("PREF_N:     %u\n", params.PREF_N);
   printf("PREF_M:     %u\n", params.PREF_M);
   printf("trace_file: %s\n", trace_file);
   printf("\n");

   // Construct cache hierarchy
   Cache* l1Cache = nullptr;
   Cache* l2Cache = nullptr;
   Cache* cacheWithPrefetch = nullptr;

   // Instantiate L1 cache
   if (params.L1_SIZE != 0) {
      l1Cache = new Cache(1, params.L1_SIZE, params.BLOCKSIZE, params.L1_ASSOC);

      // Instantiate L2 cache
      if (params.L2_SIZE !=0) {
         l2Cache = new Cache(2, params.L2_SIZE, params.BLOCKSIZE, params.L2_ASSOC);
         // Linking the caches such that L1 can access L2
         l1Cache->setNextCacheLevel(l2Cache);

         if (params.PREF_N > 0) { // Stream buffers have to be added
            // Stream Buffers has to be added to the last level of cache
            // If L2 exists, add the stream buffers to L2
            l2Cache->addStreamBuffers(params.PREF_N, params.PREF_M);
            cacheWithPrefetch = l2Cache;
         }
      }
      else if (params.PREF_N > 0) { // Stream buffers have to be added
         // Stream Buffers has to be added to the last level of cache
         // Since L2 does not exist, add the stream buffers to L1
         l1Cache->addStreamBuffers(params.PREF_N, params.PREF_M);
         cacheWithPrefetch = l1Cache;
      }
   }

   // Read requests from the trace file and proceess them
   uint32_t rwCount = 0;
   while (fscanf(fp, "%c %x\n", &rw, &addr) == 2) {	// Stay in the loop if fscanf() successfully parsed two tokens as specified.
      if (rw != 'r' && rw !='w') {
        printf("Error: Unknown request type %c.\n", rw);
	   exit(EXIT_FAILURE);
      }

      ///////////////////////////////////////////////////////
      // Issue the request to the L1 cache instance here.
      ///////////////////////////////////////////////////////
      if (l1Cache != nullptr) {
         rwCount += 1;
         debugPrint("%d=%c %x\n", rwCount, rw, addr);
         l1Cache->executeInstruction(rw, addr);
      }
   }
   // Generate output
   // Print L1 cache contents
   if (l1Cache != nullptr) {
      l1Cache->printContents();
      printf("\n");
   }
   // Print L2 cache contents
   if (l2Cache != nullptr) {
      l2Cache->printContents();
      printf("\n");
   }
   // Print Stream buffer contents if it exists
   if (cacheWithPrefetch != nullptr) {
      cacheWithPrefetch->printStreamBufferContents();
      printf("\n");
   }
   // Print Measurements
   printf("===== Measurements =====\n");
   // L1 measurements
   if (l1Cache != nullptr) {
      printf("a. L1 reads:                   %d\n", l1Cache->getReads());
      printf("b. L1 read misses:             %d\n", l1Cache->getReadMisses());
      printf("c. L1 writes:                  %d\n", l1Cache->getWrites());
      printf("d. L1 write misses:            %d\n", l1Cache->getWriteMisses());
      printf("e. L1 miss rate:               %.4f\n", l1Cache->getMissRate());
      printf("f. L1 writebacks:              %d\n", l1Cache->getWritebacks());
      printf("g. L1 prefetches:              %d\n", l1Cache->getPrefetches());
   }
   else {
      printf("a. L1 reads:                   %d\n", 0);
      printf("b. L1 read misses:             %d\n", 0);
      printf("c. L1 writes:                  %d\n", 0);
      printf("d. L1 write misses:            %d\n", 0);
      printf("e. L1 miss rate:               %.4f\n", 0.0);
      printf("f. L1 writebacks:              %d\n", 0);
      printf("g. L1 prefetches:              %d\n", 0);
   }
   if (l2Cache != nullptr) {
      printf("h. L2 reads (demand):          %d\n", l2Cache->getReads());
      printf("i. L2 read misses (demand):    %d\n", l2Cache->getReadMisses());
      printf("j. L2 reads (prefetch):        %d\n", l2Cache->getReadPrefetches());
      printf("k. L2 read misses (prefetch):  %d\n", l2Cache->getReadMissPrefetches());
      printf("l. L2 writes:                  %d\n", l2Cache->getWrites());
      printf("m. L2 write misses:            %d\n", l2Cache->getWriteMisses());
      printf("n. L2 miss rate:               %.4f\n", l2Cache->getMissRate());
      printf("o. L2 writebacks:              %d\n", l2Cache->getWritebacks());
      printf("p. L2 prefetches:              %d\n", l2Cache->getPrefetches());
   }
   else {
      printf("h. L2 reads (demand):          %d\n", 0);
      printf("i. L2 read misses (demand):    %d\n", 0);
      printf("j. L2 reads (prefetch):        %d\n", 0);
      printf("k. L2 read misses (prefetch):  %d\n", 0);
      printf("l. L2 writes:                  %d\n", 0);
      printf("m. L2 write misses:            %d\n", 0);
      printf("n. L2 miss rate:               %.4f\n", 0.0);
      printf("o. L2 writebacks:              %d\n", 0);
      printf("p. L2 prefetches:              %d\n", 0);
   }
   printf("q. memory traffic:             %d\n", memTraffic);

   // Deallocate dynamic memory allocation for creating the instances of these classes
   // This is to avoid memory leaks
   delete l1Cache;
   delete l2Cache;
   return(0);
}
