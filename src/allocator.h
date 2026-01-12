//
// Created by David Samia on 11/20/25.
//

#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>

#define MIN_BLOCK_SPLIT_SIZE 128

#define BLOCK_NAME_SIZE 23
#define ALIGN_SIZE 8

// Default whether malloc should set default values for allocated memory.
#ifndef SCRIBBLE_ALLOCATION
#define SCRIBBLE_ALLOCATION 1
#endif

// Default the scribble character to be 0xAA
// Supposed to be fastest? Need to test...
#ifndef SCRIBBLE_CHAR
#define SCRIBBLE_CHAR 0xAA
#endif

/** Struct to hold metadata for a memory region that will be further divided into blocks. */
struct MemoryRegion {
  uint64_t regionId;
  struct MemoryRegion *nextRegion;
  pthread_mutex_t lock;
};

// MemoryBlock bitmask bit values.
#define IS_FREE 0
#define IS_RED_NODE 1

/**
 * Struct holding the metadata for a memory block that has been allocated.
 * Bitmask bits:
 * 0 - isFree
 * 1 - *tree implementation only* isRedNode (red/black tree implementation)
 */
struct MemoryBlock {
  uint64_t blockId;
  struct MemoryBlock *nextBlock;
  struct MemoryBlock *prevBlock;
  struct MemoryBlock *nextFreeBlock;  // Used in the linked-list and b-tree implementation.
#if TREE_IMPLEMETATION
  struct MemoryBlock *prevFreeBlock;  // For the b-tree implementation, need a prev for left branches.
#endif
  size_t size;
  // bool isFree;
  uint8_t bitmask;
  char name[BLOCK_NAME_SIZE];
};

struct MemoryAllocator {
  // Counters
  uint64_t regionCount;
  uint64_t allocationCount;

  // Linked-lists for memory regions. Holds reference to the last allocated region (tail).
  struct MemoryRegion* regionListHead;
  struct MemoryRegion* regionListTail;
  struct MemoryBlock* freeBlockList;

  pthread_mutex_t regionListLock;
  pthread_mutex_t freeListLock;
};

typedef struct MemoryBlock MemoryBlock;
typedef struct MemoryRegion MemoryRegion;

// main malloc functions
void* mallocImpl(size_t size);
void* callocImpl(size_t objectCount, size_t objectSize);
void* reallocImpl(void* ptr, size_t newSize);
void freeImpl(void* ptr);

// internal helper functions
size_t align(size_t origSize, size_t alignment);
MemoryBlock* splitBlock(MemoryBlock* block, size_t splitSize);
MemoryBlock* mergeBlock(MemoryBlock* block);
void addFreeBlock(MemoryBlock* block);
void removeFreeBlock(const MemoryBlock* block);
MemoryBlock* findBestFreeBlock(size_t size);
int32_t freeBlockCount();
void printFreeBlocksChain();
void printBlocksChain(MemoryBlock* block);
void printBlockInfo(MemoryBlock* block);
bool isFreeBlock(const MemoryBlock* block);

// tree implementation helper functions
bool isBlockRedNode(const MemoryBlock* block);

#endif //ALLOCATOR_H
