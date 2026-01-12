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
 *  simple frees:
 *  - alloc small block -> free small block => free again
 *  - alloc large block -> free large block => free again
 *  - alloc two blocks -> free first => first is free again
 *  - alloc two blocks -> free second => second is free again
 *  - alloc many blocks -> free every other block => only freed are free, others are hash preserved
 */
TEST_START("simple free")

SUBTEST("small free", {
  const int testAllocSize = align(400, ALIGN_SIZE);
  char* testAlloc = mallocImpl(testAllocSize);
  TEST_ASSERT_NOT_NULL(testAlloc);

  MemoryBlock* testBlock = ((MemoryBlock*)testAlloc) - 1;
  TEST_ASSERT_EQUAL(testBlock->size, testAllocSize + sizeof(MemoryBlock));
  TEST_ASSERT_IS_FALSE(isFreeBlock(testBlock));

  freeImpl(testAlloc);
  TEST_ASSERT_IS_TRUE(isFreeBlock(testBlock));
});

SUBTEST("large free", {
  const int testAllocSize = align(400000, ALIGN_SIZE);
  char* testAlloc = mallocImpl(testAllocSize);
  TEST_ASSERT_NOT_NULL(testAlloc);

  MemoryBlock* testBlock = ((MemoryBlock*)testAlloc) - 1;
  TEST_ASSERT_EQUAL(testBlock->size, testAllocSize + sizeof(MemoryBlock));
  TEST_ASSERT_IS_FALSE(isFreeBlock(testBlock));

  freeImpl(testAlloc);
  TEST_ASSERT_IS_TRUE(isFreeBlock(testBlock));
});

SUBTEST("free with multiple blocks 1", {
  const int testAllocSize = align(64, ALIGN_SIZE);
  char* firstAlloc = mallocImpl(testAllocSize);
  TEST_ASSERT_NOT_NULL(firstAlloc);

  char* secondAlloc = mallocImpl(testAllocSize);
  TEST_ASSERT_NOT_NULL(secondAlloc);

  MemoryBlock* firstTestBlock = ((MemoryBlock*)firstAlloc) - 1;
  TEST_ASSERT_EQUAL(firstTestBlock->size, testAllocSize + sizeof(MemoryBlock));
  TEST_ASSERT_IS_FALSE(isFreeBlock(firstTestBlock));

  MemoryBlock* secondTestBlock = ((MemoryBlock*)secondAlloc) - 1;
  TEST_ASSERT_EQUAL(secondTestBlock->size, testAllocSize + sizeof(MemoryBlock));
  TEST_ASSERT_IS_FALSE(isFreeBlock(secondTestBlock));

  freeImpl(firstAlloc);
  TEST_ASSERT_IS_TRUE(isFreeBlock(firstTestBlock));
  TEST_ASSERT_IS_FALSE(isFreeBlock(secondTestBlock));
});

SUBTEST("free with multiple blocks 2", {
  const int testAllocSize = align(64, ALIGN_SIZE);
  char* firstAlloc = mallocImpl(testAllocSize);
  TEST_ASSERT_NOT_NULL(firstAlloc);

  char* secondAlloc = mallocImpl(testAllocSize);
  TEST_ASSERT_NOT_NULL(secondAlloc);

  MemoryBlock* firstTestBlock = ((MemoryBlock*)firstAlloc) - 1;
  TEST_ASSERT_IS_FALSE(isFreeBlock(firstTestBlock));

  MemoryBlock* secondTestBlock = ((MemoryBlock*)secondAlloc) - 1;
  TEST_ASSERT_IS_FALSE(isFreeBlock(secondTestBlock));

  freeImpl(secondAlloc);
  TEST_ASSERT_IS_FALSE(isFreeBlock(firstTestBlock));
  TEST_ASSERT_IS_TRUE(isFreeBlock(secondTestBlock));
});

TEST_END;
