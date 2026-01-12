//
// Created by David Samia on 1/11/26.
//

//
// Created by David Samia on 1/11/26.
//

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "testRunner.h"
#include "../src/allocator.h"

/*
 *  test plan:
 *  realloc smaller:
 *    - alloc block -> realloc to be smaller, but larger than MIN_SPLIT_SIZE -> new alloc small enough to use => new alloc reused split off section, original alloc is smaller
 *    - alloc block -> realloc smaller, too small to split => realloc size stayed the same (could not split)
 *    - alloc block -> realloc to 0 => block was freed
 */

TEST_START("realloc decreased size");

SUBTEST("realloc null size > 0, mallocs block correct size", {
  const int testAllocSize = align(40, ALIGN_SIZE);

  uint8_t* testAlloc = reallocImpl(NULL, testAllocSize);

  const MemoryBlock* originalTestBlock = ((MemoryBlock*)testAlloc) - 1;

  TEST_ASSERT_NOT_NULL(testAlloc);
  TEST_ASSERT_IS_FALSE(isFreeBlock(originalTestBlock));
  TEST_ASSERT_EQUAL(originalTestBlock->size, testAllocSize + sizeof(MemoryBlock));
});

SUBTEST("realloc null size <= 0, returns NULL", {
  const int testAllocSize = 0;

  uint8_t* testAlloc = reallocImpl(NULL, testAllocSize);

  TEST_ASSERT_IS_NULL(testAlloc);
});

SUBTEST("realloc 0, block was freed", {
  const int testAllocSize = align(40, ALIGN_SIZE);

  uint8_t* testAlloc = mallocImpl(testAllocSize);

  const MemoryBlock* originalTestBlock = ((MemoryBlock*)testAlloc) - 1;

  const int reallocSize = 0;
  const uint8_t* resizedTestAlloc = reallocImpl(testAlloc, reallocSize);

  TEST_ASSERT_IS_NULL(resizedTestAlloc);
  TEST_ASSERT_IS_TRUE(isFreeBlock(originalTestBlock));
});

SUBTEST("realloc too small to split, size stays the same", {
  const int testAllocSize = align(4000, ALIGN_SIZE);

  uint8_t* testAlloc = mallocImpl(testAllocSize);

  const MemoryBlock* originalTestBlock = ((MemoryBlock*)testAlloc) - 1;

  const int reallocSize = testAllocSize - MIN_BLOCK_SPLIT_SIZE + 1;
  const uint8_t* resizedTestAlloc = reallocImpl(testAlloc, reallocSize);
  const MemoryBlock* resizedTestBlock = ((MemoryBlock*)resizedTestAlloc) - 1;

  TEST_ASSERT_NOT_NULL(resizedTestAlloc);
  TEST_ASSERT_POINTER_EQUAL(resizedTestAlloc, testAlloc);
  TEST_ASSERT_EQUAL(resizedTestBlock->size, testAllocSize + sizeof(MemoryBlock));
});

SUBTEST("realloc large enough to split, size changed, next block is free", {
  const int testAllocSize = align(4000, ALIGN_SIZE);

  uint8_t* testAlloc = mallocImpl(testAllocSize);

  const MemoryBlock* originalTestBlock = ((MemoryBlock*)testAlloc) - 1;

  const int reallocSize = testAllocSize - MIN_BLOCK_SPLIT_SIZE - sizeof(MemoryBlock);
  const uint8_t* resizedTestAlloc = reallocImpl(testAlloc, reallocSize);
  const MemoryBlock* resizedTestBlock = ((MemoryBlock*)resizedTestAlloc) - 1;

  TEST_ASSERT_NOT_NULL(resizedTestAlloc);
  TEST_ASSERT_POINTER_EQUAL(resizedTestAlloc, testAlloc);
  TEST_ASSERT_EQUAL(resizedTestBlock->size, reallocSize + sizeof(MemoryBlock));
  TEST_ASSERT_NOT_NULL(resizedTestBlock->nextBlock);
  TEST_ASSERT_IS_TRUE(isFreeBlock(resizedTestBlock->nextBlock));
});

TEST_END;



