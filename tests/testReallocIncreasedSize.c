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
 *  realloc larger:
 *    next is free:
 *    - alloc small block -> realloc to be larger => alloc pointer stayed the same, size increased
 *    - alloc three small blocks -> free middle -> realloc first to be larger than 1 + 2 => alloc pointer changed, original is free, contents copied over
 *    next isn't free:
 *    - alloc two small blocks -> realloc first => alloc pointer was changed, original is free, contents copied over
 */

TEST_START("realloc increased size");

SUBTEST("next free and large enough, pointer is same", {
  const int testAllocSize = align(400, ALIGN_SIZE);
  char* testAlloc = mallocImpl(testAllocSize);
  TEST_ASSERT_NOT_NULL(testAlloc);

  const int reallocSize = 2 * testAllocSize;
  char* resizedTestAlloc = reallocImpl(testAlloc, reallocSize);

  TEST_ASSERT_POINTER_EQUAL(testAlloc, resizedTestAlloc);
});

SUBTEST("next free and large enough, correct size", {
  const int testAllocSize = align(400, ALIGN_SIZE);
  char* testAlloc = mallocImpl(testAllocSize);
  TEST_ASSERT_NOT_NULL(testAlloc);

  const MemoryBlock* testBlock = ((MemoryBlock*)testAlloc) - 1;
  TEST_ASSERT_EQUAL(testBlock->size, testAllocSize + sizeof(MemoryBlock));
  TEST_ASSERT_IS_FALSE(isFreeBlock(testBlock));

  const int reallocSize = 2 * testAllocSize;
  char* resizedTestAlloc = reallocImpl(testAlloc, reallocSize);
  const MemoryBlock* resizedTestBlock = ((MemoryBlock*)resizedTestAlloc) - 1;

  TEST_ASSERT_EQUAL(resizedTestBlock->size, reallocSize + sizeof(MemoryBlock));
});

SUBTEST("next free and large enough, not free", {
  const int testAllocSize = align(400, ALIGN_SIZE);
  char* testAlloc = mallocImpl(testAllocSize);
  TEST_ASSERT_NOT_NULL(testAlloc);

  const MemoryBlock* testBlock = ((MemoryBlock*)testAlloc) - 1;
  TEST_ASSERT_EQUAL(testBlock->size, testAllocSize + sizeof(MemoryBlock));
  TEST_ASSERT_IS_FALSE(isFreeBlock(testBlock));

  const int reallocSize = 2 * testAllocSize;
  char* resizedTestAlloc = reallocImpl(testAlloc, reallocSize);
  const MemoryBlock* resizedTestBlock = ((MemoryBlock*)resizedTestAlloc) - 1;

  TEST_ASSERT_IS_FALSE(isFreeBlock(resizedTestBlock));
});

SUBTEST("next is free but too small, different pointer", {
  const int testAllocSize = align(40, ALIGN_SIZE);
  const int testAllocCount = 3;
  char* testAllocPointers[3];

  // check allocations worked
  for (int i = 0; i < testAllocCount; i++) {
    char* testAlloc = mallocImpl(testAllocSize);
    TEST_ASSERT_NOT_NULL(testAlloc);
    testAllocPointers[i] = testAlloc;
  }

  char* testAlloc = testAllocPointers[0];

  // free middle to test reallocing prev [0]
  freeImpl(testAllocPointers[1]);

  const int reallocSize = 4 * testAllocSize;
  char* resizedTestAlloc = reallocImpl(testAlloc, reallocSize);

  TEST_ASSERT_POINTER_NOT_EQUAL(testAlloc, resizedTestAlloc);
});

SUBTEST("next is free but too small, correct size", {
  const int testAllocSize = align(40, ALIGN_SIZE);
  const int testAllocCount = 3;
  char* testAllocPointers[3];

  // check allocations worked
  for (int i = 0; i < testAllocCount; i++) {
    char* testAlloc = mallocImpl(testAllocSize);
    TEST_ASSERT_NOT_NULL(testAlloc);
    testAllocPointers[i] = testAlloc;
  }

  char* testAlloc = testAllocPointers[0];

  // free middle to test reallocing prev [0]
  freeImpl(testAllocPointers[1]);

  const int reallocSize = 4 * testAllocSize;
  char* resizedTestAlloc = reallocImpl(testAlloc, reallocSize);
  const MemoryBlock* resizedTestBlock = ((MemoryBlock*)resizedTestAlloc) - 1;

  TEST_ASSERT_EQUAL(resizedTestBlock->size, reallocSize + sizeof(MemoryBlock));
});

SUBTEST("next is free but too small, next still free and size same", {
  const int testAllocSize = align(40, ALIGN_SIZE);
  const int testAllocCount = 3;
  char* testAllocPointers[3];

  // check allocations worked
  for (int i = 0; i < testAllocCount; i++) {
    char* testAlloc = mallocImpl(testAllocSize);
    TEST_ASSERT_NOT_NULL(testAlloc);
    testAllocPointers[i] = testAlloc;
  }

  char* testAlloc = testAllocPointers[0];
  char* freedAlloc = testAllocPointers[1];
  const MemoryBlock* freedTestBlock = ((MemoryBlock*)freedAlloc) - 1;
  const size_t freedBlockSize = freedTestBlock->size;

  // free middle to test reallocing prev [0]
  freeImpl(testAllocPointers[1]);

  const int reallocSize = 4 * testAllocSize;
  char* resizedTestAlloc = reallocImpl(testAlloc, reallocSize);

  TEST_ASSERT_EQUAL(freedTestBlock->size, freedBlockSize);
  TEST_ASSERT_IS_TRUE(isFreeBlock(freedTestBlock));
});

SUBTEST("next is free but too small, contents copied over correctly", {
  const int testAllocSize = align(40, ALIGN_SIZE);
  const int testAllocCount = 3;
  char* testAllocPointers[3];
  uint32_t blockHashes[3];  // store to test that the contents are preserved after realloc...

  // check allocations worked
  for (int i = 0; i < testAllocCount; i++) {
    char* newAlloc = mallocImpl(testAllocSize);
    TEST_ASSERT_NOT_NULL(newAlloc);
    testAllocPointers[i] = newAlloc;
    for (int j = 0; j < testAllocSize; j++) {
      newAlloc[j] = (char)rand() % 255 + 1;
    }
    newAlloc[testAllocSize - 1] = '\0';
    blockHashes[i] = djb2((uint8_t*)newAlloc);
  }

  char* testAlloc = testAllocPointers[0];
  char* freedAlloc = testAllocPointers[1];

  // free middle to test reallocing prev [0]
  freeImpl(freedAlloc);

  const int reallocSize = 4 * testAllocSize;
  const char* resizedTestAlloc = reallocImpl(testAlloc, reallocSize);

  const uint32_t resizedHash = djb2((uint8_t*)resizedTestAlloc);
  const uint32_t freedHash = djb2((uint8_t*)freedAlloc);

  TEST_ASSERT_UINT8_EQUAL(resizedHash, blockHashes[0]);
  TEST_ASSERT_UINT8_EQUAL(freedHash, blockHashes[1]);
});

SUBTEST("next not free, contents copied over correctly", {
  const int testAllocSize = align(40, ALIGN_SIZE);
  const int testAllocCount = 3;
  uint8_t* testAllocPointers[3];
  uint32_t blockHashes[3];  // store to test that the contents are preserved after realloc...

  // check allocations worked
  for (int i = 0; i < testAllocCount; i++) {
    uint8_t* newAlloc = mallocImpl(testAllocSize);
    TEST_ASSERT_NOT_NULL(newAlloc);
    testAllocPointers[i] = newAlloc;
    for (int j = 0; j < testAllocSize; j++) {
      newAlloc[j] = (char)rand() % 255 + 1;
    }
    newAlloc[testAllocSize - 1] = '\0';
    blockHashes[i] = djb2(newAlloc);
  }

  uint8_t* testAlloc = testAllocPointers[0];
  const uint8_t* nextAlloc = testAllocPointers[1];

  const int reallocSize = 4 * testAllocSize;
  const uint8_t* resizedTestAlloc = reallocImpl(testAlloc, reallocSize);

  const uint32_t resizedHash = djb2(resizedTestAlloc);
  const uint32_t freedHash = djb2(nextAlloc);

  TEST_ASSERT_UINT8_EQUAL(resizedHash, blockHashes[0]);
  TEST_ASSERT_UINT8_EQUAL(freedHash, blockHashes[1]);
});

SUBTEST("next not free, correct size", {
  const int testAllocSize = align(40, ALIGN_SIZE);

  uint8_t* testAlloc = mallocImpl(testAllocSize);
  uint8_t* nextAlloc = mallocImpl(testAllocSize);

  const int reallocSize = 4 * testAllocSize;
  const uint8_t* resizedTestAlloc = reallocImpl(testAlloc, reallocSize);
  const MemoryBlock* resizedTestBlock = ((MemoryBlock*)resizedTestAlloc) - 1;

  TEST_ASSERT_EQUAL(resizedTestBlock->size, reallocSize + sizeof(MemoryBlock));
});

SUBTEST("next not free, pointer changed", {
  const int testAllocSize = align(40, ALIGN_SIZE);

  uint8_t* testAlloc = mallocImpl(testAllocSize);
  uint8_t* nextAlloc = mallocImpl(testAllocSize);

  const int reallocSize = 4 * testAllocSize;
  const uint8_t* resizedTestAlloc = reallocImpl(testAlloc, reallocSize);

  TEST_ASSERT_POINTER_NOT_EQUAL(testAlloc, resizedTestAlloc);
});

SUBTEST("next not free, is free correct", {
  const int testAllocSize = align(40, ALIGN_SIZE);

  uint8_t* testAlloc = mallocImpl(testAllocSize);
  uint8_t* nextAlloc = mallocImpl(testAllocSize);

  const MemoryBlock* originalTestBlock = ((MemoryBlock*)testAlloc) - 1;
  const MemoryBlock* nextTestBlock = ((MemoryBlock*)nextAlloc) - 1;

  const int reallocSize = 4 * testAllocSize;
  const uint8_t* resizedTestAlloc = reallocImpl(testAlloc, reallocSize);
  const MemoryBlock* resizedTestBlock = ((MemoryBlock*)resizedTestAlloc) - 1;

  TEST_ASSERT_IS_FALSE(isFreeBlock(resizedTestBlock));
  TEST_ASSERT_IS_FALSE(isFreeBlock(nextTestBlock));
  TEST_ASSERT_IS_TRUE(isFreeBlock(originalTestBlock));
});

TEST_END;



