#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdarg> // Include the cstdarg header for variable argument handling

// Address size is fixed to 32 bits
#define ADDRESS_SIZE 32
// Enable/disable debug prints using DEBUG macro
# define DEBUG 0

void debugPrint(const char* format, ...) {
   #if DEBUG
      va_list args;
      va_start(args, format);
      vprintf(format, args);
      va_end(args);
   #endif
}
// Misceallaneous functions
class Utility {
public:
    // Converts integer to hex string
    // This is needed for printing debug runs
    static std::string toHexString(int num) {
        const char hexChars[] = "0123456789abcdef";
        std::string hexString;
        for (int i = 7; i >= 0; --i) {
            int nibble = (num >> (4 * i)) & 0xF;
            hexString += hexChars[nibble];
        }
        size_t firstNonZero = hexString.find_first_not_of('0');
        if (firstNonZero != std::string::npos) {
            hexString = hexString.substr(firstNonZero);
        }
        hexString.insert(hexString.begin(), 8 - hexString.size(), ' ');
        return hexString;
    }
};

// Memory traffic variable
uint32_t memTraffic = 0;

// Memory Block structure
struct memBlock {
    uint32_t tag;
    bool dirtyBit;
    bool valid;
    uint32_t lruRank;
    uint32_t addr;
};

struct sbMemBlock {
    uint32_t tagAndIndex;
};

// Stream buffer class 
class StreamBuffer {
private:
    int M;  // size of the stream buffer
    bool valid; // indicates if a stream buffer is valid or not
    std::vector<sbMemBlock> buffer; // represents the collection of memory blocks within a stream buffer

public:
    uint32_t lruRank; // keeps track of lru rank of a stream buffer
    // Initialise all the memory blocks in a stream buffer as invalid
    StreamBuffer(int M) : M(M), valid(false) {
        buffer.resize(M);
        for (int i = 0; i < M; ++i) {
            buffer[i].tagAndIndex = -1;
        }
    }

    // Function to check the validity of a stream buffer
    bool isValid() {
        return valid;
    }

    // function to update the validity of a stream buffer
    void setValid(bool value) {
        valid = value;
    }

    // Getter for memory blocks vector
    std::vector<sbMemBlock>& getSBMemoryBlocks() {
        return this->buffer;
    }

    // Check if Stream Buffer has a memory block
    bool hasSBMemoryBlock(uint32_t tagAndIndex) {
        bool value = false;
        if (this-> isValid()) {
            for (auto& sbMemBlock : buffer) {
                if (sbMemBlock.tagAndIndex == tagAndIndex) {
                    value = true;
                    break;
                }
            }
        }
        return value;
    }
    
    // Method to find a memBlock using its pointer
    sbMemBlock* getSBMemoryBlock(uint32_t tagAndIndex) {
        for (auto& sbMemBlock : buffer) {
            if (sbMemBlock.tagAndIndex == tagAndIndex) {
                return &sbMemBlock;
            }
        }
        return nullptr; // Return nullptr if not found
    }

    // Function to get a stream buffer's content
    std::string getContent() {
        std::string value = "";
        std::vector<sbMemBlock>& sbMemBlocks = this->getSBMemoryBlocks();
        for (auto& sbMemBlock : sbMemBlocks) {
            uint32_t tagAndIndex = sbMemBlock.tagAndIndex;
            std::string hexString = Utility::toHexString(tagAndIndex);
            while (hexString.length() < 8) {
                hexString = " "+hexString;
            }
            value += hexString + " ";
        }
        return value;
    }
}; // Stream buffer class ends here

// Class to model sets within a cache
class CacheSet {
    private:
        uint32_t setCount;
        uint32_t assoc;
        std::vector<memBlock> memBlocks;

    public:
        // Constructor for CacheSet
        CacheSet(int setCount, int assoc) : setCount(setCount), assoc(assoc) {
            // Create as many memory block containers as the asscociativity
            for (int i = 0; i < assoc; ++i) {
                memBlock memoryBlock;
                memoryBlock.tag = -1;
                memoryBlock.dirtyBit = false;
                memoryBlock.valid = false;
                memoryBlock.lruRank = -1;
                memoryBlock.addr = -1;
                memBlocks.push_back(memoryBlock);
            }
        }
    // Return set count of the set
    uint32_t getSetIndex() {
        return setCount;
    }

    // Generates the set content as a string
    std::string getSetContent() {
        std::vector<memBlock> mruSortedMemBlocks = this->getMRUSortedMemoryBlocks();
        std::string value = "";
        if (!mruSortedMemBlocks.empty()) {
            for (auto& memBlock : mruSortedMemBlocks) {
                uint32_t tag = memBlock.tag;
                std::string hexString = Utility::toHexString(tag);
                while (hexString.length() < 8) {
                    hexString = " "+hexString;
                }
                if (memBlock.dirtyBit) {
                    hexString += " D";
                }
                value += hexString + " ";
            }
        }
        return value;
    }

    // Check if a memory block with a given tag and index exists in the set
    bool hasMemoryBlock(uint32_t tag) {
        for (const auto& memBlock : memBlocks) {
            if (memBlock.tag == tag) {
                return true;
            }
        }
        return false;
   }

    // Get all memory blocks in the set
    std::vector<memBlock> getMemoryBlocks() {
        return memBlocks;
    }

    // Get valid blocks sorted in order of MRU
    std::vector<memBlock> getMRUSortedMemoryBlocks() {
        // copy of memBlocks to avoid modifying the original memBlocks vector
        std::vector<memBlock> sortedMemBlocks;
        // Copy valid blocks to validMemBlocks vector
        for (const auto& block : memBlocks) {
            if (block.valid) {
                sortedMemBlocks.push_back(block); // still unsorted
            }
        }
        // custom comparator function to sort by lruRank in ascending order
        auto comparator = [](const memBlock& a, const memBlock& b) {
            return a.lruRank < b.lruRank;
        };
        // now perfrom the sort in ascending order of lruRank using the above comparator function
        std::sort(sortedMemBlocks.begin(), sortedMemBlocks.end(), comparator);
        return sortedMemBlocks;
    }

    // Get a memory block with a given tag and index from the set
    memBlock* getMemoryBlock(uint32_t tag) {
        for (auto& iterMemBlock : memBlocks) {
             if (iterMemBlock.tag == tag) {
                return const_cast<memBlock*>(&iterMemBlock);
            }
        }
        return nullptr; // Return a nullptr if not found
    }

    // Allocate a memory block. Find the first invalid memory block
    // and fill it with the requested tag
    memBlock* allocateMemoryBlock(uint32_t tag, uint32_t addr) {
        memBlock* value = nullptr;
        for(auto& iterMemBlock: memBlocks) {
            if (!iterMemBlock.valid) {
                iterMemBlock.valid = true;
                iterMemBlock.tag = tag;
                iterMemBlock.dirtyBit = false;
                iterMemBlock.addr = addr;
                value = &iterMemBlock;
                break;
            }
        }
        return value;
    }

    // Check if there is an invalid memory block within the cache set
    bool hasInvalidMemoryBlock() {
        for (const auto& memBlock : memBlocks) {
            if (!memBlock.valid) {
                return true;
            }
        }
        return false;
    }

    // Get the least recently used (LRU) memory block within the cache set
    memBlock* getLRUMemoryBlock() {
        memBlock* lruMemoryBlock = nullptr;
        bool notFoundValidBlock = true;
        // Iterate through each memBlock in the vector
        for (auto& block : memBlocks) {
            // If a memBlock is valid, update the lruMemoryBlock pointer if it has a higher lruRank
            if (block.valid) {
                if (notFoundValidBlock || block.lruRank > lruMemoryBlock->lruRank) {
                    lruMemoryBlock = &block;
                    notFoundValidBlock = false;
                }
            }
            // If there is at least one invalid memBlock, return nullptr
            else {
                return nullptr;
            }
        }
        // Return the pointer to the memBlock with the highest lruRank among valid memBlocks
        return lruMemoryBlock;
    }

    // Updates the LRU rank of all the valid blocks
    // The requested MRUMemBlock's rank becomes 0; rank of all other blocks increases by 1
    void updateLRURank(memBlock* MRUMemBlock) {
        // Set the lru rank of MRUMemBlock to zero
        MRUMemBlock->lruRank = 0;
        for (auto& memBlock : memBlocks) {
            if (&memBlock != MRUMemBlock && memBlock.valid) {
                memBlock.lruRank += 1;
            }
        }
    }

    // Evict a memory block with a given tag and index from the set
    void invalidateMemoryBlock(memBlock* lruMemBlock) {
        lruMemBlock->tag = -1;
        lruMemBlock->valid = false;
        lruMemBlock->dirtyBit = false;
        lruMemBlock->lruRank = -1;
        lruMemBlock->addr = -1;

    }
}; // class CacheSet ends

// Cache class to model any cache level l1, l2 etc.
class Cache {
private:
    uint32_t cacheLevelIndex; // stores the cache level; 1 for L1, 2 for L2 etc
    uint32_t size; // cache size
    uint32_t blocksize; // size of memory block
    uint32_t assoc; // set associativity of the cache
    uint32_t N; // number of stream buffers
    uint32_t M; // size of each stream buffer
    std::string writePolicy; // write policy; fixed as wbwa in this simulator
    uint32_t addressSize; // size of address
    uint32_t setCount; // count the number of sets based on cache size, block size and assoc
    uint32_t indexBitCount; // no. of bits that represent index
    uint32_t blockOffsetBitCount; // no. of bits that represent block offset
    uint32_t tagBitCount; // no. of bits that represent tag
    std::vector<CacheSet> sets; // vector to hold objects of set class
    std::vector<StreamBuffer> streamBuffers; // vector to hold objects of stream buffer class
    uint32_t addr; // holds the address being serviced
    struct CacheMeasurement{
        uint32_t reads;
        uint32_t readMisses;
        uint32_t writes;
        uint32_t writeMisses;
        uint32_t writebacks;
        uint32_t prefetches;
        uint32_t readsPrefetch;
        uint32_t readMissesPrefetch;
        double missRate;       
    }; CacheMeasurement cacheStats; // declare a variable of type struct CacheMeasurement to keep track of the same
    Cache* nextCacheLevel; // pointer to the next Cache object in the linked list

    // Private methods
    
    // ------------------------------------- Increment methods for cache measurements -------------------------------------
    void incrementReads() {
        cacheStats.reads++;
        this->updateMissRate();
    }

    void incrementReadMisses() {
        cacheStats.readMisses++;
        this->updateMissRate();
    }

    void incrementWrites() {
        cacheStats.writes++;
        this->updateMissRate();
    }

    void incrementWriteMisses() {
        cacheStats.writeMisses++;
        this->updateMissRate();
    }

    void incrementWriteBacks() {
        cacheStats.writebacks++;
    }

    void incrementPrefetches() {
        cacheStats.prefetches++;
    }

    void incrementReadPrefetches() {
        cacheStats.readsPrefetch++;
    }

    void incrementReadMissPrefetches() {
        cacheStats.readMissesPrefetch++;
    }

    // Updates miss rate dynamically
    // For L1:  (L1 read misses + L1 write misses)/(L1 reads + L1 writes)
    // For L2: (L2 read misses that did not originate from L1 prefetches,
    //          excluding such L2 read misses that hit in the stream buffers if L2 prefetch unit is enabled)
    //         ----------------------------------------------------------------------------------------------
    //         (L2 reads that did not originate from L1 prefetches, i.e., L1 read misses + L1 write misses)
    void updateMissRate() {
        double value = 0.0;
        uint32_t cachelevel = this->getCacheLevel();
        if (cachelevel == 1) {
            if ((this->getReads() + this->getWrites()) != 0) {
                value = double(this->getReadMisses() + this->getWriteMisses()) / double(this->getReads() + this->getWrites());
            }
        }
        if (cachelevel == 2) {
            if ((this->getReads()) != 0) {
                value = double(this->getReadMisses()) / double(this->getReads());
            }
        }        
    cacheStats.missRate = value;
    }

    // Generic method to extract bits between a start and end position
    uint32_t extractBits(uint32_t num, uint32_t start, uint32_t end) {
        // Create a mask to extract the desired bits
        uint32_t mask = (1 << (end - start + 1)) - 1;
        mask <<= start;
        // Use the mask to extract the bits
        uint32_t value = num & mask;
        value >>= start;
        return value;
    }

public:
    Cache (
        uint32_t cacheLevelIndex, 
        uint32_t size, 
        uint32_t blocksize, 
        uint32_t assoc,
        uint32_t N=0,
        uint32_t M=0,
        const std::string& writePolicy = "wbwa", 
        uint32_t addressSize = ADDRESS_SIZE)
        : 
        cacheLevelIndex(cacheLevelIndex), 
        size(size), 
        blocksize(blocksize), 
        assoc(assoc),
        N(N),
        M(M),
        writePolicy(writePolicy), 
        addressSize(addressSize), 
        nextCacheLevel(nullptr) {
        
        // Address bits calculation
        setCount = size / (assoc * blocksize);
        indexBitCount = static_cast<uint32_t>(log2(setCount));
        blockOffsetBitCount = static_cast<uint32_t>(log2(blocksize));
        tagBitCount = addressSize - indexBitCount - blockOffsetBitCount;
        // Add empty 'set' to vector 'sets'
        // Reserve memory for as many sets as the setCount
        sets.reserve(setCount);
        for (uint32_t everySet = 0; everySet < setCount; ++everySet) {
            // Directly add the set object to the vector without having to 
            // temporarily create an instance of set class and then push to vector sets
            sets.emplace_back(everySet, this->assoc); 
        }
        // Initialize cache measurement params
        cacheStats.reads = 0;
        cacheStats.readMisses = 0;
        cacheStats.writes = 0;
        cacheStats.writeMisses = 0;
        cacheStats.writebacks = 0;
        cacheStats.prefetches = 0;
        cacheStats.readsPrefetch = 0;
        cacheStats.readMissesPrefetch = 0;
        cacheStats.missRate = 0.0;
    }
    
    // Add stream buffers if they are configured to be present
    void addStreamBuffers(uint32_t sbSize, uint32_t mbSize) {
        this->N= sbSize;
        this->M = mbSize;
        if (N > 0) {
            this->streamBuffers.reserve(N);
            for (uint32_t everySB = 0; everySB < N; ++everySB) {
                this->streamBuffers.emplace_back(this->M);
            }
        }
    }

    // ------------------------------------- Methods for getting cache parameters measurements -------------------------------------
    uint32_t getReads() {
        return cacheStats.reads;
    }

    uint32_t getReadMisses() {
        return cacheStats.readMisses;
    }

    uint32_t getWrites() {
        return cacheStats.writes;
    }

    uint32_t getWriteMisses() {
        return cacheStats.writeMisses;
    }

    uint32_t getWritebacks() {
        return cacheStats.writebacks;
    }

    uint32_t getPrefetches() {
        return cacheStats.prefetches;
    }
    
    uint32_t getReadPrefetches() {
        //return cacheStats.readsPrefetch;
        return 0;
    }

    uint32_t getReadMissPrefetches() {
        //return cacheStats.readMissesPrefetch;
        return 0;
    }

    double getMissRate() {     
        return cacheStats.missRate;
    }

    // ------------------------------------- Methods for accessing the next level of cache from the current level -------------------------------------
    // Function to set the next cache in the linked list
    void setNextCacheLevel(Cache* next) {
        nextCacheLevel = next;
    }

    // Function to get the next cache in the linked list
    Cache* getNextCacheLevel() {
        return nextCacheLevel;
    }
   
    // Getter for cache's current level
    uint32_t getCacheLevel() {
        return cacheLevelIndex;
    }

    // ------------------------------------- Methods for printing output -------------------------------------
    // Generate debug tabs
    std::string generateTabs() {
        std::string tabs;
        for (u_int32_t i = 0; i < this->getCacheLevel(); ++i) {
            tabs +="\t";
        }
        return tabs;
    }

    // Function to print the cache configuration
    void printContents() {
        printf("===== L%d contents =====\n",this->getCacheLevel());
        for (uint32_t setCount = 0; setCount < this->getSetCount(); ++setCount) {
            // set      setCount: 
            printf("set %6d: ", setCount);
            CacheSet* set = this->getSet(setCount);
            std::vector<memBlock> memBlocks = set->getMRUSortedMemoryBlocks();
            // iterate over memBlocks
            for (auto& memBlock : memBlocks) {
                if (memBlock.valid) {
                    if (memBlock.dirtyBit) {
                        // tag needs 8 cols; single space and D for dirty bit
                        printf("%8x D", memBlock.tag);
                    }
                    else {
                        // tag needs 8 cols
                        printf("%8x  ", memBlock.tag);
                    }
                }
            }
            printf("\n");
        }
    }
    
    // Function to print stream buffer contents if it exists
    void printStreamBufferContents() {
        if (this->N !=0 ) {
            printf("===== Stream Buffer(s) contents =====\n");
            std::vector<StreamBuffer> streamBuffers = this->getMRUSortedStreamBuffers();
            for (auto& streamBuffer : streamBuffers) {
                printf("%s\n", streamBuffer.getContent().c_str());
            }
        }
    }

    // ------------------------------------- Methods for sets -------------------------------------
    // Fucntion to get the #sets in this cache
    uint32_t getSetCount() const {
        return setCount;
    }

    // Function to return a vector of objects of set class
    const std::vector<CacheSet>& getSets() const {
        return sets;
    }

    // Searches and returns the pointer to the set whose index is same as the target index
    CacheSet* getSet(uint32_t index) {
        for (auto& iteratorSet:sets) {
            if (iteratorSet.getSetIndex() == index) {
                return &iteratorSet;
            }
        }
        return nullptr;
    }

    // ------------------------------------- Methods for splitting requested address into tag, index, offset and its combinations -------------------------------------
    // Returns the index as integer of the requested address
    uint32_t getIndex(uint32_t addr) {
            uint32_t start = blockOffsetBitCount;
            uint32_t end = blockOffsetBitCount + indexBitCount - 1;
            uint32_t index = this->extractBits(addr, start, end);
            return index;
    }

    // Returns the tag as integer of the requested address
    uint32_t getTag(uint32_t addr) {
            uint32_t start = blockOffsetBitCount + indexBitCount;
            uint32_t end  = addressSize - 1;
            uint32_t tag = this->extractBits(addr, start, end);
            return tag;
    }

    // Returns the tag and index as integer of the requested address without any shifting
    uint32_t getTagAndIndex(uint32_t addr) {
            uint32_t start = blockOffsetBitCount;
            uint32_t end = addressSize - 1;
            uint32_t tagAndIndex = this->extractBits(addr, start, end);
            return tagAndIndex;
    }
    
    // Concatenate Tag and Index of a block and lshift it block offset times
    uint32_t getTagllIndex(uint32_t tag, uint32_t index) {
        return (tag << (this->indexBitCount + this->blockOffsetBitCount)) | (index << this->blockOffsetBitCount);
    }

    // Returns the block offset as integer of the requested address
    uint32_t getBlockOffset(uint32_t addr) {
            uint32_t start = 0;
            uint32_t end = blockOffsetBitCount - 1;
            uint32_t blockOffset = this->extractBits(addr, start, end);
            return blockOffset;
    }
    // ------------------------------------- Methods for prefetch capability -------------------------------------
    // get LRU stream buffer
    StreamBuffer* getLRUStreamBuffer() {
        StreamBuffer* lruSB = nullptr;
        int maxLRURank = std::numeric_limits<int>::min();  // Initialize to minimum possible value.
        bool foundInvalidStreamBuffer = false;

        for (auto& streamBuffer : streamBuffers) {
            if (!streamBuffer.isValid()) {
                foundInvalidStreamBuffer = true;
                lruSB = &streamBuffer;
                break;  // Found an invalid stream buffer, no need to check further.
            }
            else {
                int currentLRURank = streamBuffer.lruRank;
                if (currentLRURank > maxLRURank) {
                    maxLRURank = currentLRURank;
                    lruSB = &streamBuffer;
                }
            }
        }

        if (foundInvalidStreamBuffer || lruSB != nullptr) {
            return lruSB;
        }
        else {
            // Handle the case where all stream buffers are valid.
            // Return the buffer with the highest LRU rank.
            for (auto& streamBuffer : streamBuffers) {
                int currentLRURank = streamBuffer.lruRank;
                if (currentLRURank > maxLRURank) {
                    maxLRURank = currentLRURank;
                    lruSB = &streamBuffer;
                }
            }
            return lruSB;
        }
    }

    // Function to update lruRank of StreamBuffer objects
    void updateSBLRURank(StreamBuffer* streamBuffer) {
        // Set the lruRank of the provided streamBuffer pointer to zero
        streamBuffer->lruRank = 0;

        // Update the rank of all other valid stream buffers by 1
        for (auto& buffer : streamBuffers) {
            if (&buffer != streamBuffer && buffer.isValid()) {
                buffer.lruRank++;
            }
        }
    }

    // Function to get MRU sorted stream buffers
    std::vector<StreamBuffer> getMRUSortedStreamBuffers() {
        // Create a vector to store valid StreamBuffer objects
        std::vector<StreamBuffer> validStreamBuffers;

        // Iterate through input streamBuffers and copy only valid StreamBuffer objects
        for (auto& buffer : this->streamBuffers) {
            if (buffer.isValid()) {
                validStreamBuffers.push_back(buffer);
            }
        }
        // Bubble Sort: Compare and swap elements based on lruRank in ascending order
        for (size_t i = 0; i < validStreamBuffers.size() - 1; ++i) {
            for (size_t j = 0; j < validStreamBuffers.size() - i - 1; ++j) {
                if (validStreamBuffers[j].lruRank > validStreamBuffers[j + 1].lruRank) {
                    // Swap elements if they are in the wrong order
                    StreamBuffer temp = validStreamBuffers[j];
                    validStreamBuffers[j] = validStreamBuffers[j + 1];
                    validStreamBuffers[j + 1] = temp;
                }
            }
        }
        // Return the sorted vector of valid StreamBuffer objects
        return validStreamBuffers;
    }

    // Prefetch blocks into stream buffer
    void prefetchBlocksIntoStreamBuffer(uint32_t tagAndIndex, uint32_t streamSize, StreamBuffer* targetStreamBuffer = nullptr) {
        if (targetStreamBuffer == nullptr) {
            targetStreamBuffer = this->getLRUStreamBuffer();
        }
        std::vector<sbMemBlock>& targetSBMemBlocks = targetStreamBuffer->getSBMemoryBlocks();
        uint32_t startIndex = (M - streamSize) % M;
        tagAndIndex++;
        for (uint32_t i = startIndex; i < M; ++i) {
            targetSBMemBlocks[i].tagAndIndex = tagAndIndex;
            tagAndIndex++;
            // Increment prefetch counter
            this->incrementPrefetches();
            // Increment memory traffic counter as well because prefetch will get the data from memory
            memTraffic++;
        }

        // mark it valid if it was invalid earlier as it has now prefetched memory blocks
        if(!(targetStreamBuffer->isValid())) {
            targetStreamBuffer->setValid(true);
        }
        // Update targetStreamBuffer lru rank
        updateSBLRURank(targetStreamBuffer);
    }
    
    // Function to get the most recently used (MRU) stream buffer with the lowest lruRank
    StreamBuffer* getMRUStreamBuffer() {
        StreamBuffer* mruStreamBuffer = nullptr;
        uint32_t lowestRank = std::numeric_limits<uint32_t>::max(); // Initialize to maximum possible value

        for (auto& buffer : this->streamBuffers) {
            if (buffer.isValid() && buffer.lruRank < lowestRank) {
                lowestRank = buffer.lruRank;
                mruStreamBuffer = &buffer;
            }
        }
        return mruStreamBuffer;
    }

    // Transfer block from stream buffer to cache
    void transferBlockfromStreamBuffer(uint32_t tagAndIndex) {
        // Find all the stream buffers containing the specific sbMemBlock with matching tagAndIndex
        std::vector<StreamBuffer*> matchingStreamBuffers;
        for (auto& streamBuffer : this->streamBuffers) {
            bool found = false;
            for (auto& block : streamBuffer.getSBMemoryBlocks()) {
                if (block.tagAndIndex == tagAndIndex) {
                    found = true;
                    break;
                }
            }
            if (found) {
                matchingStreamBuffers.push_back(&streamBuffer);
            }
        }
        // Choose the MRU stream buffer
        StreamBuffer* mruStreamBuffer = this->getMRUStreamBuffer();
        // Find the element in MRU buffer with matching tagAndIndex and calculate its index
        size_t elementIndex = 0;
        bool found = false;
        for (auto& sbmemblock : mruStreamBuffer->getSBMemoryBlocks()) {
            if (sbmemblock.tagAndIndex == tagAndIndex) {
                found = true;
                break;
            }
            elementIndex++;
        }
        // Shift the elements after the found element up and prefetch for the freed up blocks
        if (found) {
            std::vector<sbMemBlock>& mruMemBlocks = mruStreamBuffer->getSBMemoryBlocks();
            for (uint32_t i = elementIndex+1; i < M; ++i) {
                mruMemBlocks[i-elementIndex-1].tagAndIndex = mruMemBlocks[i].tagAndIndex;
            }
            prefetchBlocksIntoStreamBuffer(tagAndIndex+M-elementIndex-1, elementIndex+1, mruStreamBuffer);
        }
    }
    
    // Function to stay in sync with the demand stream of the cache when there is a hit in both the cache and the stream buffers
    void stayInSyncWithDemandStream(uint32_t tagAndIndex) {
        transferBlockfromStreamBuffer(tagAndIndex);
    }

    // ------------------------------------- Methods for handling cache operation -------------------------------------
    // Handle cache hit
    void processCacheHit(char instr, uint32_t addr, uint32_t tag, uint32_t index, memBlock* targetMemBlock, CacheSet* targetSet, bool streamBufferHit=false) {
        // Fetch the hit block
        targetMemBlock = targetSet->getMemoryBlock(tag);
        // ***** Debug statements begin
        debugPrint("%sL%d: %6s: set %6d: %s\n",this->generateTabs().c_str(), this->getCacheLevel(), std::string("before").c_str(), index, targetSet->getSetContent().c_str());
        // ***** Debug statements end
        if (instr == 'r') { // Read hit
            // Increment read counter
            this->incrementReads();
        }
        else if (instr == 'w') { // Write hit
            // Set dirty bit because write was requested
            targetMemBlock->dirtyBit = true;
            // Increment write counter
            this->incrementWrites();
        }
        if (streamBufferHit) {
            // Scenario 4: Hits in the cache and hits in the prefetch unit as well
            stayInSyncWithDemandStream(this->getTagAndIndex(addr));
        }
        // Update LRU rank of the hit block and other valid blocks in the set
        targetSet->updateLRURank(targetMemBlock);
        // ***** Debug statements begin
        debugPrint("%sL%d: %6s: set %6d: %s\n",this->generateTabs().c_str(), this->getCacheLevel(), std::string("after").c_str(), index, targetSet->getSetContent().c_str());
        if (streamBufferHit) {
            for (auto& streamBuffer : this->streamBuffers) {
                if (streamBuffer.isValid()){
                    debugPrint("\t\t\tSB: %s\n", streamBuffer.getContent().c_str());
                }
        }
        }
        // ***** Debug statements end
    }

    // Handle cache miss
    void processCacheMiss(char instr, uint32_t addr, uint32_t tag, uint32_t index, memBlock* targetMemBlock, CacheSet* targetSet, bool streamBufferHit=false) {
        // Handle cache miss logic here
        //  There is no memory block in this set with the requested tag
        // ***** Debug statements begin
        debugPrint("%sL%d: %6s: set %6d: %s\n",this->generateTabs().c_str(), this->getCacheLevel(), std::string("before").c_str(), index, targetSet->getSetContent().c_str());
        // ***** Debug statements end
        if (!streamBufferHit && instr == 'r') { // Read miss excluding those that hit in stream buffers if prefetch unit is present
            this->incrementReadMisses();
            }
        else if (!streamBufferHit && instr == 'w') { // Write miss excluding those that hit in stream buffers if prefetch unit is present
            this->incrementWriteMisses();
        }
        // First evict then allocate
        if (targetSet->hasInvalidMemoryBlock()) { // At least one invalid memory block in the set
            // Issue request to the next level
            Cache* nextCache = this->getNextCacheLevel();
                if (nextCache != nullptr) { // Next cache level exists
                    // Send read instruction to next level
                    uint32_t tagllIndex = this->getTagllIndex(tag, index);
                    nextCache->executeInstruction('r', tagllIndex);
                    // Following scenario (prefetching at a level higher than the lowermost level) is out of scope; leaving the code for improvising later
                    // if (!streamBufferHit) {
                    //     // Scenario #1:
                    //     // prefetch the next M consecutive memory blocks into Cache
                    //     // debugPrint("\t\t\tScenario #1 invalid memory block next cache level exists\n");
                    //     uint32_t tagAndIndex = this->getTagAndIndex(addr);
                    //     prefetchBlocksIntoStreamBuffer(tagAndIndex, M);
                    // }
                    // else {
                    //         // Scenario #2:
                    //         // instead of making request to the next level of cache,
                    //         // copy the request block X from the Stream buffer into Cache
                    //         // debugPrint("\t\t\tScenario #2 dirty lru memory block next cache level exists\n");
                    //         uint32_t tagAndIndex = this->getTagAndIndex(addr);
                    //         transferBlockfromStreamBuffer(tagAndIndex);
                    //     }
                }
                else { // Accessing main memory
                    if (!streamBufferHit) {
                        memTraffic++;
                        // Scenario #1:
                        // prefetch the next M consecutive memory blocks into Cache
                        // debugPrint("\t\t\tScenario #1 invalid memory block next cache level not exists\n");
                        uint32_t tagAndIndex = this->getTagAndIndex(addr);
                        prefetchBlocksIntoStreamBuffer(tagAndIndex, M);
                    }
                    else {
                            // Scenario #2:
                            // instead of making request to the next level of cache,
                            // copy the request block X from the Stream buffer into Cache
                            // debugPrint("\t\t\tScenario #2 dirty lru memory block next cache level exists\n");
                            uint32_t tagAndIndex = this->getTagAndIndex(addr);
                            transferBlockfromStreamBuffer(tagAndIndex);
                        }
                }
        }
        else { // Check if LRUMemBlock is valid. You get invalid block in case there are existing invalid blocks in the set
            memBlock* lruMemBlock = targetSet->getLRUMemoryBlock();
            // Check if dirty bit is set
            if (lruMemBlock != nullptr) {
                if (lruMemBlock->dirtyBit) { // LRU block has the dirty bit set
                    // construct a similar address like value from tag and index
                    // of LRU memblock ignoring the block offset bits
                    // it is safe to ignore block offset bits because 
                    // block size is same at all levels
                    uint32_t lruTag = lruMemBlock->tag;
                    uint32_t lrutagllIndex = this->getTagllIndex(lruTag, index);
                    // Fetch next level in cache hierarchy
                    Cache* nextCache = this->getNextCacheLevel();
                    if (nextCache != nullptr) {
                        // Issue a write instruction to the next level
                        nextCache->executeInstruction('w', lrutagllIndex);
                        this->incrementWriteBacks();
                        targetSet->invalidateMemoryBlock(lruMemBlock);
                        // Following scenario (prefetching at a level higher than the lowermost level) is out of scope; leaving the code for improvising later
                        // if (!streamBufferHit) {
                        //     // Send request to the next level of cache
                        //     uint32_t tagllIndex = this->getTagllIndex(tag, index);
                        //     nextCache->executeInstruction('r', tagllIndex);
                        //     // Scenario #1: 
                        //     // prefetch the next M consecutive memory blocks into Cache
                        //     // debugPrint("\t\t\tScenario #1 dirty lru memory block next cache level exists\n");
                        //     uint32_t tagAndIndex = this->getTagAndIndex(addr);
                        //     prefetchBlocksIntoStreamBuffer(tagAndIndex, M);
                        // }
                        // else {
                        //     // Scenario #2:
                        //     // instead of making request to the next level of cache,
                        //     // copy the request block X from the Stream buffer into Cache
                        //     // debugPrint("\t\t\tScenario #2 dirty lru memory block next cache level exists\n");
                        //     uint32_t tagAndIndex = this->getTagAndIndex(addr);
                        //     transferBlockfromStreamBuffer(tagAndIndex);
                        // }
                    }
                    else { // this is the last level of cache. next is main memory
                        this->incrementWriteBacks();
                        // Update memory traffic counter
                        memTraffic++;
                        targetSet->invalidateMemoryBlock(lruMemBlock);
                        if (!streamBufferHit) {
                            // Again update memory traffic because now the actual request needs to be serviced after the write back
                            // But since this is a case of miss, the block needs to be allocated from main memory
                            memTraffic++;
                            // Scenario #1: 
                            // prefetch the next M consecutive memory blocks into Cache
                            // debugPrint("\t\t\tScenario #1 dirty lru memory block next cache level not exists\n");
                            uint32_t tagAndIndex = this->getTagAndIndex(addr);
                            prefetchBlocksIntoStreamBuffer(tagAndIndex, M); 
                        }
                        else {
                            // Scenario #2:
                            // instead of making request to the next level of cache,
                            // copy the request block X from the Stream buffer into Cache
                            uint32_t tagAndIndex = this->getTagAndIndex(addr);
                            // debugPrint("\t\t\tScenario #2 dirty lru memory block next cache level not exists\n");
                            transferBlockfromStreamBuffer(tagAndIndex);
                        }
                    }
                }
                else { // LRU memblock does not have the dirty bit
                    targetSet->invalidateMemoryBlock(lruMemBlock);
                    Cache* nextCache = this->getNextCacheLevel();
                    if (nextCache != nullptr) {
                        // Send read instruction to next level
                        uint32_t tagllIndex = this->getTagllIndex(tag, index);
                        nextCache->executeInstruction('r', tagllIndex);
                        // Following scenario (prefetching at a level higher than the lowermost level) is out of scope; leaving the code for improvising later
                        // if (!streamBufferHit) {
                        //     // Scenario #1: 
                        //     // prefetch the next M consecutive memory blocks into Cache
                        //     // debugPrint("\t\t\tScenario #1 not dirty lru memory block next cache level exists\n");
                        //     uint32_t tagAndIndex = this->getTagAndIndex(addr);
                        //     prefetchBlocksIntoStreamBuffer(tagAndIndex, M);
                        // }
                        // else {
                        //     // Scenario #2:
                        //     // instead of making request to the next level of cache,
                        //     // copy the request block X from the Stream buffer into Cache
                        //     uint32_t tagAndIndex = this->getTagAndIndex(addr);
                        //     // debugPrint("\t\t\tScenario #2 not dirty lru memory block next cache level exists\n");
                        //     transferBlockfromStreamBuffer(tagAndIndex);
                        // }
                    }
                    else {
                        // Scenario #1: 
                        // prefetch the next M consecutive memory blocks into Cache
                        if (!streamBufferHit) {
                            // Accessing from main memory the original request after invalidating LRU block
                            memTraffic++;
                            // debugPrint("\t\t\tScenario #1 not dirty lru memory block next cache level not exists\n");
                            uint32_t tagAndIndex = this->getTagAndIndex(addr);
                            prefetchBlocksIntoStreamBuffer(tagAndIndex, M);
                        }
                        else {
                            // Scenario #2:
                            // instead of making request to the next level of cache,
                            // copy the request block X from the Stream buffer into Cache
                            uint32_t tagAndIndex = this->getTagAndIndex(addr);
                            // debugPrint("\t\t\tScenario #2 not dirty lru memory block next cache level not exists\n");
                            transferBlockfromStreamBuffer(tagAndIndex);
                        }
                    }
                }
            }
        }
        // Allocate missed memory block at set
        memBlock* allocatedBlock = targetSet->allocateMemoryBlock(tag, addr);
        targetSet->updateLRURank(allocatedBlock);
        if (instr == 'r') { 
            // Increment read counter
            this->incrementReads();
            // Read request fulfilled
        }
        else if (instr == 'w') {
            // Set dirty bits
            allocatedBlock->dirtyBit = true;
            // Increment write counter
            this->incrementWrites();
            // Write request fulfilled
        }
        // ***** Debug statements begin
        debugPrint("%sL%d: %6s: set %6d: %s\n",this->generateTabs().c_str(), this->getCacheLevel(), std::string("after").c_str(), index, targetSet->getSetContent().c_str());
        for (auto& streamBuffer : this->streamBuffers) {
            if (streamBuffer.isValid()) {
                debugPrint("\t\t\tSB: %s\n", streamBuffer.getContent().c_str());
            }
        }
        // ***** Debug statements end
    }

    // Handles execution of instrction and address received from the cpu trace
    void executeInstruction(char instr, uint32_t addr) {
        this->addr = addr;
        uint32_t tag = this->getTag(addr);
        uint32_t index = this->getIndex(addr);
        uint32_t tagAndIndex = this->getTagAndIndex(addr);
        // ***** Debug statements begin
        debugPrint("%sL%d: %c %x (tag=%x index=%d)\n",this->generateTabs().c_str(), this->getCacheLevel(), instr, addr, tag, index);
        // ***** Debug statements end

        bool cacheHit = false;
        bool streamBufferHit = false;
        for (auto& streamBuffer : streamBuffers) {
            if (streamBuffer.isValid()) {
                if (streamBuffer.hasSBMemoryBlock(tagAndIndex)) {
                    streamBufferHit = true;
                    break;
                }
            }
        }
        // Fetch the set matching the index of the address
        CacheSet* targetSet = this->getSet(index);
        // We have to fetch the target memory block where the cache would hit/miss
        memBlock* targetMemBlock = nullptr;
        if (targetSet != nullptr) { // If we find a set == index
            cacheHit = targetSet->hasMemoryBlock(tag);
            if (!cacheHit) { // Cache Miss
                // debugPrint("\t$$$ Cache Hit False stream buffer hit %d\n", static_cast<int>(streamBufferHit));
                processCacheMiss(instr, addr, tag, index, targetMemBlock, targetSet, streamBufferHit);
            }
            else { // Cache Hit
                // debugPrint("\t$$$ Cache Hit True stream buffer hit %d\n", streamBufferHit);
                processCacheHit(instr, addr, tag, index, targetMemBlock, targetSet, static_cast<int>(streamBufferHit));
                if (!streamBufferHit) {
                    // Scenarion #3:
                    // do nothing wrt the stream buffer
                }
                else {
                    // Scenarion #4: 
                    // manage the stream buffer same as Scenario #2
                    // no transfer from stream buffer to cache
                }
            }
        }
        else { // If we don't find a set == index
            printf("Could not find calculated set for request %c %x\n", instr, this->addr);
        }
    }
}; // Class Cahe ends here
