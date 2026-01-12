//
// Created by David Samia on 1/2/26.
//

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "testRunner.h"
#include "../src/allocator.h"

/*
 *  test plan:
 *  todo: make test plan
 */

TEST_START("thread safety");

SUBTEST("small malloc", {
  //todo: make this thread safety related
  const int testAllocSize = align(400, ALIGN_SIZE);
  char* testAlloc = mallocImpl(testAllocSize);
  TEST_ASSERT_NOT_NULL(testAlloc);

  const MemoryBlock* testBlock = ((MemoryBlock*)testAlloc) - 1;
  TEST_ASSERT_EQUAL(testBlock->size, testAllocSize + sizeof(MemoryBlock));
  TEST_ASSERT_IS_FALSE(testBlock->isFree);
});

TEST_END;



