
add_executable(testMalloc tests/testSimpleMalloc.c
  allocator.h
  logger.h
  allocator.c
  tests/testRunner.h
)

add_executable(testFree tests/testSimpleFree.c
  allocator.h
  logger.h
  allocator.c
  tests/testRunner.h
)

add_executable(testMerge tests/testMerge.c
  allocator.h
  logger.h
  allocator.c
  tests/testRunner.h
)