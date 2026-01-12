
cmake_minimum_required(VERSION 3.30)
project(memory_allocator)

add_executable(testMalloc tests/testSimpleMalloc.c
  src/allocator.h
  src/logger.h
  src/allocator.c
  tests/testRunner.h
)

add_executable(testFree tests/testSimpleFree.c
  src/allocator.h
  src/logger.h
  src/allocator.c
  tests/testRunner.h
)

add_executable(testReallocIncreasedSize tests/testReallocIncreasedSize.c
  src/allocator.h
  src/logger.h
  src/allocator.c
  tests/testRunner.h
)

add_executable(testReallocDecreasedSize tests/testReallocDecreasedSize.c
  src/allocator.h
  src/logger.h
  src/allocator.c
  tests/testRunner.h
)

add_executable(testSimpleMerge tests/testSimpleMerge.c
  src/allocator.h
  src/logger.h
  src/allocator.c
  tests/testRunner.h
)

add_executable(testAdvancedMerge tests/testAdvancedMerge.c
  src/allocator.h
  src/logger.h
  src/allocator.c
  tests/testRunner.h
)

target_compile_definitions(testMalloc PUBLIC
  LOGGER=0
  LOGGER_COLOR=1
  LOG_LOG=1
  LOG_WARN=1
  LOG_ERROR=1
  AUTO_NEW_LINE=1
  SCRIBBLE_ALLOCATION=1
  SCRIBBLE_CHAR=0xAA
)

target_compile_definitions(testFree PUBLIC
  LOGGER=0
  LOGGER_COLOR=1
  LOG_LOG=1
  LOG_WARN=1
  LOG_ERROR=1
  AUTO_NEW_LINE=1
)

target_compile_definitions(testSimpleMerge PUBLIC
  LOGGER=0
  LOGGER_COLOR=1
  LOG_LOG=1
  LOG_WARN=1
  LOG_ERROR=1
  AUTO_NEW_LINE=1
)

target_compile_definitions(testAdvancedMerge PUBLIC
  LOGGER=0
  LOGGER_COLOR=1
  LOG_LOG=1
  LOG_WARN=1
  LOG_ERROR=1
  AUTO_NEW_LINE=1
)

