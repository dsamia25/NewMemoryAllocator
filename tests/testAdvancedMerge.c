//
// Created by David Samia on 1/2/26.
//

#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

#include "testRunner.h"
#include "../src/allocator.h"

/*
 *  separated this test so that other test allocations will not mess with reused pointers.
 *
 *  test plan:
 *  - alloc large block -> free large block -> alloc many small blocks => large block reused and subdivided
 */
TEST_START("free block merging")

SUBTEST("reuse block 3, split large block into smaller parts", {
  const int testAllocSize = align(4000, ALIGN_SIZE);
  char* testAlloc = mallocImpl(testAllocSize);
  TEST_ASSERT_NOT_NULL(testAlloc);

  MemoryBlock* testBlock = ((MemoryBlock*)testAlloc) - 1;
  TEST_ASSERT_EQUAL(testBlock->size, testAllocSize + sizeof(MemoryBlock));
  TEST_ASSERT_IS_FALSE(isFreeBlock(testBlock));

  const void* blockEnd = (void*)testBlock + testBlock->size;
  // printf("testBlock - %p - (testAlloc %p - %p)\n", testBlock, testAlloc, blockEnd);

  freeImpl(testAlloc);
  TEST_ASSERT_IS_TRUE(isFreeBlock(testBlock));

  const int smallAllocSize = align(10, ALIGN_SIZE);
  for (int i = 0; i < (testAllocSize + sizeof(MemoryBlock)) / (smallAllocSize + sizeof(MemoryBlock)) - 1; i++) {
    char* smallAlloc = mallocImpl(smallAllocSize);
    TEST_ASSERT_NOT_NULL(smallAlloc);
    // printf("%d - %p\n", i, smallAlloc);

    // check that the small alloc is within the bounds of the original block
    const bool gtetTestBlockStart = (void*)smallAlloc >= (void*)testBlock;
    const bool ltetTestBlockEnd = (void*)smallAlloc <= blockEnd;
    TEST_ASSERT_IS_TRUE(gtetTestBlockStart);
    TEST_ASSERT_IS_TRUE(ltetTestBlockEnd);
  }
});

TEST_END;
