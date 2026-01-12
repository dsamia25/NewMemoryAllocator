//
// Created by David Samia on 12/30/25.
//

#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

#include "testRunner.h"
#include "../src/allocator.h"

/*
 *  test plan:
 *  - alloc two blocks -> free first then second => both merged and free
 *  - alloc two blocks -> free second then first => both merged and free
 *  - alloc small block -> free block -> alloc small block again => second alloc is reused
 *  - alloc small block -> alloc large block -> free small block -> alloc small block again => small is reused
 *  - alloc large block -> free large block -> alloc many small blocks => large block reused and subdivided
 */
TEST_START("free block merging")

SUBTEST("merge two blocks 1", {
  const int testAllocSize = align(400, ALIGN_SIZE);
  char* testAlloc1 = mallocImpl(testAllocSize);
  TEST_ASSERT_NOT_NULL(testAlloc1);

  char* testAlloc2 = mallocImpl(testAllocSize);
  TEST_ASSERT_NOT_NULL(testAlloc2);

  // make sure prev allocs only merge into each other...
  char* interruptAlloc = mallocImpl(1);
  TEST_ASSERT_NOT_NULL(interruptAlloc);

  MemoryBlock* testBlock1 = ((MemoryBlock*)testAlloc1) - 1;
  TEST_ASSERT_EQUAL(testBlock1->size, testAllocSize + sizeof(MemoryBlock));
  TEST_ASSERT_IS_FALSE(isFreeBlock(testBlock1));

  MemoryBlock* testBlock2 = ((MemoryBlock*)testAlloc2) - 1;
  TEST_ASSERT_EQUAL(testBlock2->size, testAllocSize + sizeof(MemoryBlock));
  TEST_ASSERT_IS_FALSE(isFreeBlock(testBlock2));

  const size_t mergedSize = testBlock1->size + testBlock2->size;

  // free 1 then 2
  freeImpl(testAlloc1);
  freeImpl(testAlloc2);

  TEST_ASSERT_IS_TRUE(isFreeBlock(testBlock1));
  TEST_ASSERT_UINT64_EQUAL(testBlock1->size, mergedSize);

  freeImpl(interruptAlloc);

  const int32_t freeBlocks = freeBlockCount();
  TEST_ASSERT_UINT32_EQUAL(freeBlocks, 1);
});

SUBTEST("merge two blocks 2", {
  const int testAllocSize = align(400, ALIGN_SIZE);
  char* testAlloc1 = mallocImpl(testAllocSize);
  TEST_ASSERT_NOT_NULL(testAlloc1);

  char* testAlloc2 = mallocImpl(testAllocSize);
  TEST_ASSERT_NOT_NULL(testAlloc2);

  // make sure prev allocs only merge into each other...
  char* interruptAlloc = mallocImpl(1);
  TEST_ASSERT_NOT_NULL(interruptAlloc);

  MemoryBlock* testBlock1 = ((MemoryBlock*)testAlloc1) - 1;
  TEST_ASSERT_EQUAL(testBlock1->size, testAllocSize + sizeof(MemoryBlock));
  TEST_ASSERT_IS_FALSE(isFreeBlock(testBlock1));

  MemoryBlock* testBlock2 = ((MemoryBlock*)testAlloc2) - 1;
  TEST_ASSERT_EQUAL(testBlock2->size, testAllocSize + sizeof(MemoryBlock));
  TEST_ASSERT_IS_FALSE(isFreeBlock(testBlock2));

  const size_t mergedSize = testBlock1->size + testBlock2->size;

  // free 2 then 1
  freeImpl(testAlloc2);
  freeImpl(testAlloc1);

  TEST_ASSERT_IS_TRUE(isFreeBlock(testBlock1));
  TEST_ASSERT_UINT64_EQUAL(testBlock1->size, mergedSize);

  freeImpl(interruptAlloc);

  const int32_t freeBlocks = freeBlockCount();
  TEST_ASSERT_UINT32_EQUAL(freeBlocks, 1);
});

SUBTEST("merge three blocks, middle merge", {
  int32_t freeBlocks = freeBlockCount();
  TEST_ASSERT_UINT32_EQUAL(freeBlocks, 1);

  const int testAllocSize = align(400, ALIGN_SIZE);
  char* testAlloc1 = mallocImpl(testAllocSize);
  TEST_ASSERT_NOT_NULL(testAlloc1);

  freeBlocks = freeBlockCount();
  TEST_ASSERT_UINT32_EQUAL(freeBlocks, 1);
  printFreeBlocksChain();

  char* testAlloc2 = mallocImpl(testAllocSize);
  TEST_ASSERT_NOT_NULL(testAlloc2);

  freeBlocks = freeBlockCount();
  TEST_ASSERT_UINT32_EQUAL(freeBlocks, 1);
  printFreeBlocksChain();

  char* testAlloc3 = mallocImpl(testAllocSize);
  TEST_ASSERT_NOT_NULL(testAlloc3);

  freeBlocks = freeBlockCount();
  TEST_ASSERT_UINT32_EQUAL(freeBlocks, 1);
  printFreeBlocksChain();

  // make sure prev allocs only merge into each other...
  char* interruptAlloc = mallocImpl(testAllocSize);
  TEST_ASSERT_NOT_NULL(interruptAlloc);

  MemoryBlock* testBlock1 = ((MemoryBlock*)testAlloc1) - 1;
  TEST_ASSERT_EQUAL(testBlock1->size, testAllocSize + sizeof(MemoryBlock));
  TEST_ASSERT_IS_FALSE(isFreeBlock(testBlock1));

  MemoryBlock* testBlock2 = ((MemoryBlock*)testAlloc2) - 1;
  TEST_ASSERT_EQUAL(testBlock2->size, testAllocSize + sizeof(MemoryBlock));
  TEST_ASSERT_IS_FALSE(isFreeBlock(testBlock2));

  MemoryBlock* testBlock3 = ((MemoryBlock*)testAlloc3) - 1;
  TEST_ASSERT_EQUAL(testBlock3->size, testAllocSize + sizeof(MemoryBlock));
  TEST_ASSERT_IS_FALSE(isFreeBlock(testBlock3));

  MemoryBlock* interruptBlock = ((MemoryBlock*)interruptAlloc) - 1;
  TEST_ASSERT_EQUAL(interruptBlock->size, testAllocSize + sizeof(MemoryBlock));
  TEST_ASSERT_IS_FALSE(isFreeBlock(interruptBlock));

  const size_t mergedSize = testBlock1->size + testBlock2->size + testBlock3->size;

  // free side ones first, then middle
  freeImpl(testAlloc1);
  freeImpl(testAlloc3);

  TEST_ASSERT_IS_TRUE(isFreeBlock(testBlock1));
  TEST_ASSERT_IS_TRUE(isFreeBlock(testBlock3));
  TEST_ASSERT_IS_FALSE(isFreeBlock(testBlock2));
  TEST_ASSERT_IS_FALSE(isFreeBlock(interruptBlock));

  freeImpl(testAlloc2);

  TEST_ASSERT_UINT64_EQUAL(testBlock1->size, mergedSize);

  freeImpl(interruptAlloc);

  freeBlocks = freeBlockCount();
  TEST_ASSERT_UINT32_EQUAL(freeBlocks, 1);
  printFreeBlocksChain();
});

SUBTEST("reuse block 1", {
  const int testAllocSize = align(400, ALIGN_SIZE);
  char* testAlloc1 = mallocImpl(testAllocSize);
  TEST_ASSERT_NOT_NULL(testAlloc1);

  MemoryBlock* testBlock1 = ((MemoryBlock*)testAlloc1) - 1;
  TEST_ASSERT_EQUAL(testBlock1->size, testAllocSize + sizeof(MemoryBlock));
  TEST_ASSERT_IS_FALSE(isFreeBlock(testBlock1));

  freeImpl(testAlloc1);

  char* testAlloc2 = mallocImpl(testAllocSize);
  TEST_ASSERT_NOT_NULL(testAlloc2);

  MemoryBlock* testBlock2 = ((MemoryBlock*)testAlloc2) - 1;
  TEST_ASSERT_EQUAL(testBlock2->size, testAllocSize + sizeof(MemoryBlock));
  TEST_ASSERT_IS_FALSE(isFreeBlock(testBlock2));

  TEST_ASSERT_POINTER_EQUAL((void*)testBlock1, (void*)testBlock2);

  freeImpl(testAlloc2);
});

SUBTEST("reuse block 2, large block following original block", {
  const int testAllocSize = align(400, ALIGN_SIZE);
  char* testAlloc1 = mallocImpl(testAllocSize);
  TEST_ASSERT_NOT_NULL(testAlloc1);

  char* testAlloc2 = mallocImpl(2 * testAllocSize);
  TEST_ASSERT_NOT_NULL(testAlloc2);

  MemoryBlock* testBlock1 = ((MemoryBlock*)testAlloc1) - 1;
  TEST_ASSERT_EQUAL(testBlock1->size, testAllocSize + sizeof(MemoryBlock));
  TEST_ASSERT_IS_FALSE(isFreeBlock(testBlock1));

  MemoryBlock* testBlock2 = ((MemoryBlock*)testAlloc2) - 1;
  TEST_ASSERT_EQUAL(testBlock2->size, 2 * testAllocSize + sizeof(MemoryBlock));
  TEST_ASSERT_IS_FALSE(isFreeBlock(testBlock2));

  freeImpl(testAlloc1);

  char* testAlloc3 = mallocImpl(testAllocSize);
  TEST_ASSERT_NOT_NULL(testAlloc3);

  MemoryBlock* testBlock3 = ((MemoryBlock*)testAlloc3) - 1;
  TEST_ASSERT_EQUAL(testBlock3->size, testAllocSize + sizeof(MemoryBlock));
  TEST_ASSERT_IS_FALSE(isFreeBlock(testBlock3));

  TEST_ASSERT_POINTER_EQUAL((void*)testBlock1, (void*)testBlock3);

  freeImpl(testAlloc2);
  freeImpl(testAlloc3);
});

TEST_END;
