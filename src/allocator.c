//
// Created by David Samia on 11/20/25.
//

#include "allocator.h"

#include <string.h>

#include "sys/mman.h"
#include "logger.h"

#if SCRIBBLE_ALLOCATION
#define SCRIBBLE(blockStart, scribbleChar)\
  memset(memoryBlock + 1, scribbleChar, memoryBlock->size - sizeof(MemoryBlock));
#endif

#ifndef SCRIBBLE
#define SCRIBBLE ;
#endif

static struct MemoryAllocator memoryAllocator;

inline size_t align(const size_t origSize, const size_t alignment) {
  size_t newSize = (origSize / alignment) * alignment;
  if (origSize % alignment != 0) {
    newSize += alignment;
  }
  return newSize;
}

void initMemoryAllocator(struct MemoryAllocator* allocator) {
  allocator->regionCount = 0;
  allocator->allocationCount = 0;
  allocator->regionListHead = NULL;
  allocator->regionListTail = NULL;
  allocator->freeBlockList = NULL;
}

/**
 * Frees the input pointer.
 * Has double free protection, does nothing if the block is already free.
 * Attempts to merge the block into the previous one if possible. Will add the
 * block to the cache to be reused if merging isn't possible.
 * @param ptr The pointer to free.
 */
void freeImpl(void* ptr) {
  if (ptr != NULL) {
    MemoryBlock* metadata = (MemoryBlock*)(ptr) - 1;
    if (isFreeBlock(metadata)) {
      ERROR("Tried to free freed memory block (%llu, %s)...", metadata->blockId, metadata->name);
      return;
    }
    // merge into previous block or add to the free cache
    const MemoryBlock* blockHeader = mergeBlock(metadata);
    if (blockHeader == metadata) {
      metadata->bitmask |= (1 << IS_FREE);
      addFreeBlock(metadata);
    }
  }
}

/**
 * Merges this block with its free neighbors.
 * Returns the header of the merged block.
 * If the returned header is the same as the input block, either a right merge or no merge occurred.
 * If the returned header is different, then a left merge occurred and the input block is no longer valid.
 * @param block The block to merge into the previous block
 * @return
 */
inline MemoryBlock* mergeBlock(MemoryBlock* block) {
  MemoryBlock* blockHeader = block;
  const MemoryBlock* nextBlock = block->nextBlock;
  if (nextBlock != NULL && isFreeBlock(nextBlock)) {
    LOG("RIGHT MERGE - %p with next block %p", block, nextBlock);
    block->nextBlock = nextBlock->nextBlock;
    if (block->nextBlock != NULL) {
      block->nextBlock->prevBlock = block;
    }
    block->size += nextBlock->size;
    removeFreeBlock(nextBlock); // no longer valid pointer as it's merged into the input block
  }
  MemoryBlock* prevBlock = block->prevBlock;
  if (prevBlock != NULL && isFreeBlock(prevBlock)) {
    LOG("LEFT MERGE - %p with prev block %p", block, prevBlock);
    prevBlock->nextBlock = block->nextBlock;
    if (prevBlock->nextBlock != NULL) {
      prevBlock->nextBlock->prevBlock = prevBlock;
    }
    prevBlock->size += block->size;
    blockHeader = prevBlock;  // input block will no longer be valid as it's a part of the prev now
  }
  return blockHeader;
}

void* mallocImpl(const size_t size) {
  LOG("Malloc request - %zu bytes...", size);

  const size_t alignedSize = align(size, ALIGN_SIZE);
  MemoryBlock* memoryBlock = findBestFreeBlock(alignedSize + sizeof(MemoryBlock));
  MemoryBlock* remainderBlock;
  if (memoryBlock != NULL) {
    removeFreeBlock(memoryBlock);
    remainderBlock = splitBlock(memoryBlock, alignedSize);
    if (remainderBlock != NULL) {
      addFreeBlock(remainderBlock);
    }
    SCRIBBLE(memoryBlock + 1, SCRIBBLE_CHAR);
    memoryBlock->bitmask &= ~(1 << IS_FREE);
    return memoryBlock + 1;
  }

  // Could not reuse existing memory, must mmap new memory.
  const size_t pageSize = getpagesize();
  const size_t mapSize = align(alignedSize, pageSize);
  MemoryRegion* newRegion = mmap(
    NULL,
    mapSize,
    PROT_READ | PROT_WRITE,
    MAP_PRIVATE | MAP_ANONYMOUS,
    -1,
    0
  );

  if (newRegion == MAP_FAILED) {
    ERROR("Could not map memory region (%zu bytes)...", mapSize);
    return NULL;
  }

  // Update region linked list.
  newRegion->regionId = memoryAllocator.regionCount++;
  pthread_mutex_lock(&(memoryAllocator.regionListLock));
  if (memoryAllocator.regionListHead == NULL) {
    memoryAllocator.regionListHead = newRegion;
  } else {
    memoryAllocator.regionListTail->nextRegion = newRegion;
  }
  memoryAllocator.regionListTail = newRegion;
  pthread_mutex_unlock(&(memoryAllocator.regionListLock));

  LOG("New region %llu (%lu bytes)...", newRegion->regionId, mapSize);

  // Adding 1 to newRegion moves the reference pointer past the metadata (actual memory region).
  memoryBlock = (MemoryBlock*)(newRegion + 1);
  memoryBlock->size = mapSize - sizeof(MemoryRegion); // Calculated from where the region metadata ends (just the page size)
  memoryBlock->bitmask |= (1 << IS_FREE);
  memoryBlock->blockId = memoryAllocator.allocationCount++;

  LOG("New block %llu (%lu bytes) (free? = %d)", memoryBlock->blockId, memoryBlock->size, isFreeBlock(memoryBlock));

  // Split the new block to perfect size. If there is excess, cache it for reuse later.
  remainderBlock = splitBlock(memoryBlock, alignedSize);
  if (remainderBlock != NULL) {
    addFreeBlock(remainderBlock);
  }

  LOG("Returning (%p) (id = %llu)", memoryBlock, memoryBlock->blockId);

  SCRIBBLE(memoryBlock + 1, SCRIBBLE_CHAR);

  // memoryBlock->isFree = false;
  memoryBlock->bitmask &= ~(1 << IS_FREE);
  return memoryBlock + 1;
}

/**
 * Resizes an allocation to be
 * @param ptr the current block to resize
 * @param newSize the new desired size for the block
 * @return the new allocation pointer (can be different from the original!!!)
 */
void* reallocImpl(void* ptr, const size_t newSize) {
  // handle null input case, give the desired size (malloc)
  if (ptr == NULL) {
    if (newSize <= 0) {
      return NULL;
    }
    return mallocImpl(newSize);
  }

  // actual realloc now...
  const size_t alignedSize = align(newSize, ALIGN_SIZE);
  const size_t totalDesiredSize = alignedSize + sizeof(MemoryBlock);
  MemoryBlock* ptrHeader = (MemoryBlock*)ptr - 1;
  if (totalDesiredSize > ptrHeader->size) {
    // best case is to just merge in next block if available
    const MemoryBlock* nextBlock = ptrHeader->nextBlock;
    const size_t mergedWithNextSize = ptrHeader->size + nextBlock->size;
    if (nextBlock != NULL && isFreeBlock(nextBlock) && mergedWithNextSize >= totalDesiredSize) {
      // merge the next block into this one
      ptrHeader->nextBlock = nextBlock->nextBlock;
      ptrHeader->size += nextBlock->size;
      removeFreeBlock(nextBlock);

      // split off excess
      MemoryBlock* remainderBlock = splitBlock(ptrHeader, alignedSize);
      if (remainderBlock != NULL) {
        addFreeBlock(remainderBlock);
      }
      // position didnt change :)
      return ptr;
    }

    // cant merge into next, need to alloc a new point entirely :(
    void* newPtr = mallocImpl(newSize);
    if (newPtr == NULL) {
      ERROR("Could not realloc %zu bytes", newSize);
      return NULL;
    }
    memcpy(newPtr, ptr, ptrHeader->size - sizeof(MemoryBlock));
    freeImpl(ptr);
    return newPtr;
  }

  // just free if desired size is 0
  if (newSize == 0) {
    freeImpl(ptr);
    return NULL;
  }

  // if the new size is smaller than the current size...
  // split off excess if possible, return current ptr
  MemoryBlock* remainderBlock = splitBlock(ptrHeader, alignedSize);
  if (remainderBlock != NULL) {
    addFreeBlock(remainderBlock);
  }
  return ptr;
}

/**
 * Splits a memory block in two with one block being a desired size and the other having the remainder.
 * +----------------+    +--------+--------+
 * | Block1         | -> | Block1 | Block2 |
 * +----------------+    +--------+--------+
 * @param block The block to split in two.
 * @param splitSize The desired first segment size. The split block will have the remainder.
 * @return The pointer to the split block or NULL if unable to split.
 */
MemoryBlock* splitBlock(MemoryBlock *block, const size_t splitSize) {
  const size_t fullDesiredSize = splitSize + sizeof(MemoryBlock);
  if (block->size < fullDesiredSize) {
    LOG("Block size %zu < Full desired size %zu (%zu + %zu)...", block->size, fullDesiredSize, sizeof(MemoryBlock), splitSize);
    return NULL;
  }
  const size_t minSplitBlockSize = sizeof(MemoryBlock) + MIN_BLOCK_SPLIT_SIZE;
  const size_t remainingSize = block->size - fullDesiredSize;
  if (remainingSize < minSplitBlockSize) {
    LOG("Remaining size %zu (%lu - %zu) < min size %zu (%zu + %d)...", remainingSize, block->size, fullDesiredSize, minSplitBlockSize, sizeof(MemoryBlock), MIN_BLOCK_SPLIT_SIZE);
    return NULL;
  }
  LOG("Starting split... block (%llu, %p) %lu -> %lu", block->blockId, block, block->size, fullDesiredSize);
  MemoryBlock *remainderBlock = (void*)(block) + fullDesiredSize;
  remainderBlock->blockId = memoryAllocator.allocationCount++;
  remainderBlock->size = remainingSize;
  remainderBlock->bitmask |= (1 << IS_FREE);
  remainderBlock->nextBlock = block->nextBlock;
  remainderBlock->prevBlock = block;
  remainderBlock->nextFreeBlock = NULL;
  block->nextBlock = remainderBlock;
  block->size = fullDesiredSize;
  LOG("Split complete... remainderBlock (%llu, %p) %lu", remainderBlock->blockId, remainderBlock, remainderBlock->size);
  return remainderBlock;
}

/**
 * Caches a memory block to be reused later.
 * @param block The block to add to the reusable block cache.
 */
void addFreeBlock(MemoryBlock* block) {
  LOG("Adding block (%llu, %p) to free cache...", block->blockId, block);

  pthread_mutex_lock(&(memoryAllocator.freeListLock));
  // linked list implementation
  if (memoryAllocator.freeBlockList != NULL) {
    block->nextFreeBlock = memoryAllocator.freeBlockList;
  }
  memoryAllocator.freeBlockList = block;
  pthread_mutex_unlock(&(memoryAllocator.freeListLock));
}

/**
 * Removes a block from the reusable block cache if it's there.
 * @param block The block to remove from the cache.
 */
void removeFreeBlock(const MemoryBlock* block) {
  LOG("Removing block (%llu, %p) from free cache...", block->blockId, block);
  pthread_mutex_lock(&(memoryAllocator.freeListLock));
  // linked list implementation
  const uint64_t blockId = block->blockId;
  MemoryBlock* currBlock = memoryAllocator.freeBlockList;
  if (currBlock->blockId == blockId) {
    memoryAllocator.freeBlockList = currBlock->nextFreeBlock;
    pthread_mutex_unlock(&(memoryAllocator.freeListLock));
    return;
  }
  while (currBlock->nextFreeBlock != NULL) {
    if (currBlock->nextFreeBlock->blockId == blockId) {
      MemoryBlock* blockToRemove = currBlock->nextFreeBlock;
      currBlock->nextFreeBlock = blockToRemove->nextFreeBlock;
      blockToRemove->nextFreeBlock = NULL;
      pthread_mutex_unlock(&(memoryAllocator.freeListLock));
      return;
    }
    currBlock = currBlock->nextFreeBlock;
  }
  pthread_mutex_unlock(&(memoryAllocator.freeListLock));
  ERROR("Block (%llu) not found in cache. How did we get here?", block->blockId);
}

/**
 * Finds the best-fitting memory block of the desired size.
 * @param size The minimum desired size of the memory block.
 * @return A pointer to a memory block of at least the desired size or NULL.
 */
inline MemoryBlock* findBestFreeBlock(const size_t size) {
  MemoryBlock* currBlock = memoryAllocator.freeBlockList;
  MemoryBlock* bestBlock = NULL;
  while (currBlock != NULL) {
    if (currBlock->size > size && (bestBlock == NULL || currBlock->size < bestBlock->size)) {
      bestBlock = currBlock;
    }
    else if (currBlock->size == size) {
      return currBlock;
    }
    currBlock = currBlock->nextFreeBlock;
  }
  return bestBlock;
}

inline int32_t freeBlockCount() {
  int32_t count = 0;
  MemoryBlock* currFreeBlock = memoryAllocator.freeBlockList;
  while (currFreeBlock != NULL) {
    if (currFreeBlock->nextFreeBlock == currFreeBlock) {
      ERROR("Self-loop found in free block list. Block id (%llu, %p) -> (%llu, %p)", currFreeBlock->blockId, currFreeBlock, currFreeBlock->nextFreeBlock->blockId, currFreeBlock->nextFreeBlock);
      break;
    }
    currFreeBlock = currFreeBlock->nextFreeBlock;
    count++;
  }
  return count;
}

void printFreeBlocksChain() {
  LOGP("*** FREE BLOCKS START ***");
  MemoryBlock* block = memoryAllocator.freeBlockList;
  do {
    printBlockInfo(block);
  } while ((block = block->nextFreeBlock) != NULL);
  LOGP("*** FREE BLOCKS END ***");
}

void printBlocksChain(MemoryBlock* block) {
  LOGP("*** BLOCKS START ***");
  do {
    printBlockInfo(block);
  } while ((block = block->nextBlock) != NULL);
  LOGP("*** BLOCKS END ***");
}

void printBlockInfo(MemoryBlock* block) {
  LOG("\n*** Block (%llu, %p) ***\n(%llu, %p) <-- (%llu, %p) --> (%llu, %p)", block->blockId, block, (block->prevBlock != NULL ? block->prevBlock->blockId : 0), (block -> prevBlock != NULL ? block->prevBlock : 0), block->blockId, block, (block->nextBlock != NULL ? block->nextBlock->blockId : 0), (block -> nextBlock != NULL ? block->nextBlock : 0));
}

inline bool isFreeBlock(const MemoryBlock* block) {
  return block->bitmask & (1 << IS_FREE);
}

inline bool isBlockRedNode(const MemoryBlock* block) {
  return block->bitmask & (1 << IS_RED_NODE);
}


