//
// Created by David Samia on 12/28/25.
//

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "testRunner.h"
#include "../src/allocator.h"

/*
 *  test plan:
 *  simple frees:
 *  - alloc small block => not null, not free, is correct size
 *  - alloc many small blocks => all not null, not free, are correct size
 *  - alloc large block => not null, not free, is correct size
 *  - alloc block -> copy in string => string stored in memory
 *  - alloc large block -> create hash -> another alloc -> recreate hash on original alloc => hash has not changed
 *  - alloc block -> check that each byte matches the scribble pattern
 */

TEST_START("basic malloc");

SUBTEST("small malloc", {
  const int testAllocSize = align(400, ALIGN_SIZE);
  char* testAlloc = mallocImpl(testAllocSize);
  TEST_ASSERT_NOT_NULL(testAlloc);

  MemoryBlock* testBlock = ((MemoryBlock*)testAlloc) - 1;
  TEST_ASSERT_EQUAL(testBlock->size, testAllocSize + sizeof(MemoryBlock));
  TEST_ASSERT_IS_FALSE(isFreeBlock(testBlock));
});

SUBTEST("many small mallocs", {
  const int testAllocSize = 40;
  const int testAllocCount = 10;
  char* testAllocPointers[10];

  // check allocations worked
  for (int i = 0; i < testAllocCount; i++) {
    char* testAlloc = mallocImpl(testAllocSize);
    TEST_ASSERT_NOT_NULL(testAlloc);
    testAllocPointers[i] = testAlloc;
  }

  // check for corruption
  for (int i = 0; i < testAllocCount; i++) {
    char* testAlloc = testAllocPointers[i];
    TEST_ASSERT_NOT_NULL(testAlloc);

    MemoryBlock* testBlock = ((MemoryBlock*)testAlloc) - 1;
    TEST_ASSERT_EQUAL(testBlock->size, testAllocSize + sizeof(MemoryBlock));
    TEST_ASSERT_IS_FALSE(isFreeBlock(testBlock));
  }
});

SUBTEST("large malloc", {
  const int testAllocSize = 100000;
  char* testAlloc = mallocImpl(testAllocSize);
  TEST_ASSERT_NOT_NULL(testAlloc);

  MemoryBlock* testBlock = ((MemoryBlock*)testAlloc) - 1;
  TEST_ASSERT_EQUAL(testBlock->size, testAllocSize + sizeof(MemoryBlock));
  TEST_ASSERT_IS_FALSE(isFreeBlock(testBlock));
});

SUBTEST("hello world", {
  const int testAllocSize = 1000;
  char* testAlloc = mallocImpl(testAllocSize);
  TEST_ASSERT_NOT_NULL(testAlloc);

  char* testStr = "hello world!";
  strncpy(testAlloc, testStr, testAllocSize);

  MemoryBlock* testBlock = ((MemoryBlock*)testAlloc) - 1;
  TEST_ASSERT_EQUAL(testBlock->size, testAllocSize + sizeof(MemoryBlock));
  TEST_ASSERT_IS_FALSE(isFreeBlock(testBlock));
  TEST_ASSERT_STRING_EQUAL(testAlloc, testStr, testAllocSize);
});

SUBTEST("preserved hash", {
  const int testAllocSize = 1000;
  uint8_t* testAlloc = mallocImpl(testAllocSize);
  TEST_ASSERT_NOT_NULL(testAlloc);

  for (int i = 0; i < testAllocSize; i++) {
    testAlloc[i] = rand() % 255 + 1;
  }
  testAlloc[testAllocSize - 1] = 0;

  const uint32_t hash = djb2(testAlloc);

  // alloc next slot
  uint8_t* nextAlloc = mallocImpl(testAllocSize);
  TEST_ASSERT_NOT_NULL(nextAlloc);

  // make sure that the original block hasn't been modified
  const uint32_t secondHash = djb2(testAlloc);
  TEST_ASSERT_UINT32_EQUAL(hash, secondHash);
});

SUBTEST("scribble", {

  if (SCRIBBLE_ALLOCATION) {
    const int testAllocSize = align(400, ALIGN_SIZE);
    char* testAlloc = mallocImpl(testAllocSize);
    TEST_ASSERT_NOT_NULL(testAlloc);

    MemoryBlock* testBlock = ((MemoryBlock*)testAlloc) - 1;
    TEST_ASSERT_EQUAL(testBlock->size, testAllocSize + sizeof(MemoryBlock));
    TEST_ASSERT_IS_FALSE(isFreeBlock(testBlock));

    for (int i = 0; i < testBlock->size - sizeof(MemoryBlock); i++) {
      const char c = testAlloc[i];
      TEST_ASSERT_UINT8_EQUAL((uint8_t)c, (uint8_t)SCRIBBLE_CHAR);
    }
  } else {
    TEST_ASSERT_PASS;
  }
});

TEST_END;



