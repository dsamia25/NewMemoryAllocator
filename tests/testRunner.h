//
// Created by David Samia on 12/28/25.
//

#ifndef MEMORY_ALLOCATOR_TESTRUNNER_H
#define MEMORY_ALLOCATOR_TESTRUNNER_H

#include <stdint.h>

// pretty colors
#define TEST_COLOR_PURPLE "\033[0;35m"
#define TEST_COLOR_GREEN "\033[0;32m"
#define TEST_COLOR_YELLOW "\033[1;33m"
#define TEST_COLOR_RED "\033[1;31m"
#define TEST_COLOR_BLUE "\033[0;4;34m"
#define TEST_COLOR_RESET "\033[0m"
#define TEST_COLOR_BRIGHT_YELLOW "\033[1;93m"

// test success variables
int totalTests = 0;
int passedTests = 0;
int subtestResult = 0;

// test helper functions. implemented at bottom of file
void testFlush(void);
uint32_t djb2(const uint8_t* str);

/**
 * Sets up test variables and the main script. Make sure no other main function is initialized in the build.
 * Leaves an open scopt '{'. Needs to be closed using TEST_END or manually adding a '}' after.
 * @param testName Prints the test name
 */
#define TEST_START(testName)\
  int main(int argc, char** argv) {\
    if (isatty(STDOUT_FILENO)) {\
      fprintf(stdout, "\n+----------------------------------------------------------\n| %s test\n| %s%s:%s%d\n+----------------------------------------------------------\n\nstarting tests...\n", \
        testName,\
        TEST_COLOR_BLUE, __FILE__, TEST_COLOR_RESET, __LINE__\
      );\
    }
  // close scope using TEST_END

/**
 * Closes the scope of the main function started by TEST_START.
 * Sets the function return value to 0 for passing all tests and 1
 * when failing any tests. Hopefully that's useful for scripting
 * these together.
 */
#define TEST_END\
    bool didPassTest = passedTests == totalTests;\
    if (isatty(STDOUT_FILENO)) {\
      fprintf(stdout, "\ntests complete. compiling results...\n\n%s*** - test results - ***%s\n-->%s %d/%d (%.1f%%)\n%s--> %s%s%s\n",\
        TEST_COLOR_BRIGHT_YELLOW, TEST_COLOR_RESET, (didPassTest ? TEST_COLOR_GREEN : TEST_COLOR_RED),\
        passedTests, totalTests, (totalTests > 0 ? 100 : 100.0 * passedTests / totalTests),\
        TEST_COLOR_RESET, (didPassTest ? TEST_COLOR_GREEN : TEST_COLOR_RED), (didPassTest ? "PASS :)" : "FAIL :("), TEST_COLOR_RESET\
      );\
    }\
    return passedTests == totalTests ? 0 : 1;\
  }

/**
 * Creates the scope around a self-contained subtest. Each subtest
 * needs to be passed for the whole test script to pass.
 */
#define SUBTEST(subtestName, ...)\
  {\
    if (isatty(STDOUT_FILENO)) {\
      fprintf(stdout, "\n%s### %d: %s - ###%s\n",\
        TEST_COLOR_YELLOW, ++totalTests, subtestName, TEST_COLOR_RESET\
      );\
    }\
    subtestResult = 0;\
    testFlush();\
    __VA_ARGS__\
    testFlush();\
    if (subtestResult == 0) {\
      passedTests++;\
      fprintf(stdout, "--> %sPASS :)%s\n",\
        TEST_COLOR_GREEN, TEST_COLOR_RESET\
      );\
    } else {\
      fprintf(stdout, "--> %sFAIL :(%s\n",\
        TEST_COLOR_RED, TEST_COLOR_RESET\
      );\
    }\
  };

#define TEST_DYNAMIC_OUTPUT(testStatusColor, testStatus, fmt, ...)\
do { \
  if (isatty(STDERR_FILENO)) {\
    fprintf(stderr, "%s%s%s:%d:%s %s%s: " fmt "\n", \
      TEST_COLOR_BLUE, __FILE__, TEST_COLOR_RESET, \
      __LINE__, \
      testStatusColor, testStatus, TEST_COLOR_RESET,\
      __VA_ARGS__);\
    break;\
  }\
} while (0);

#define TEST_FAIL_OUTPUT(fmt, ...) TEST_DYNAMIC_OUTPUT(TEST_COLOR_RED, "ASSERT FAILED", fmt, __VA_ARGS__)


#define TEST_ASSERT_POINTER_EQUAL(actual, expected)\
  if (actual != expected) {\
    TEST_FAIL_OUTPUT("actual (%p) != expected (%p)", actual, expected)\
    subtestResult++;\
  }

#define TEST_ASSERT_POINTER_NOT_EQUAL(actual, expected)\
  if (actual == expected) {\
    TEST_FAIL_OUTPUT("actual (%p) == expected (%p)", actual, expected)\
    subtestResult++;\
  }

#define TEST_ASSERT_UINT64_EQUAL(actual, expected)\
  if (actual != expected) {\
    TEST_FAIL_OUTPUT("actual (%lu) != expected (%lu)", actual, expected)\
    subtestResult++;\
  }

#define TEST_ASSERT_UINT32_EQUAL(actual, expected)\
  if (actual != expected) {\
    TEST_FAIL_OUTPUT("actual (%u) != expected (%u)", actual, expected);\
    subtestResult++;\
  }

#define TEST_ASSERT_UINT8_EQUAL(actual, expected)\
  if (actual != expected) {\
    TEST_FAIL_OUTPUT("actual (%u) != expected (%u)", actual, expected);\
    subtestResult++;\
  }

// todo: add versions for uint16/8, int64/32/16/8
// todo: make generic version that checks for types and uses appropriate version? fancy but not necessary

#define TEST_ASSERT_FAIL subtestResult++;
#define TEST_ASSERT_PASS subtestResult = 0;

#define TEST_ASSERT_IS_TRUE(actual)\
  if (!actual) {\
    TEST_FAIL_OUTPUT("%d is false...", actual);\
    subtestResult++;\
  }

#define TEST_ASSERT_IS_FALSE(actual)\
  if (actual) {\
    TEST_FAIL_OUTPUT("%d is true...", actual);\
    subtestResult++;\
  }

#define TEST_ASSERT_IS_NULL(actual)\
  if (actual != NULL) {\
    TEST_FAIL_OUTPUT("%p is not null...", actual);\
    subtestResult++;\
  }

#define TEST_ASSERT_NOT_NULL(actual)\
  if (actual == NULL) {\
    TEST_FAIL_OUTPUT("%p is null...", actual);\
    subtestResult++;\
  }

#define TEST_ASSERT_EQUAL(actual, expected)\
  if (actual != expected) {\
    TEST_FAIL_OUTPUT("actual (%lu) != expected (%lu)", actual, expected);\
    subtestResult++;\
  }

#define TEST_ASSERT_CHAR_EQUAL(actual, expected)\
  if (actual != expected) {\
    TEST_FAIL_OUTPUT("actual (%c) != expected (%c)", (uint8_t)actual, (uint8_t)expected);\
    subtestResult++;\
  }

#define TEST_ASSERT_STRING_EQUAL(actual, expected, bufferLength)\
  if (strncmp(actual, expected, bufferLength) != 0) {\
    TEST_FAIL_OUTPUT("actual (%s) != expected (%s)", actual, expected);\
    subtestResult++;\
  }

#define TEST_ASSERT_STRING_NOT_EQUAL(actual, expected, bufferLength)\
  if (strncmp(actual, expected, bufferLength) == 0) {\
    TEST_FAIL_OUTPUT("actual (%s) != expected (%s)", actual, expected);\
    subtestResult++;\
  }


/* DJB2 Hash. Credit: Dan Bernstein */
inline uint32_t djb2(const uint8_t* str)
{
  uint32_t hash = 5381;
  int c;

  while ((c = *str++)) {
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
  }

  return hash;
}

inline void testFlush()
{
  fflush(stdout);
  fflush(stderr);
}

#endif //MEMORY_ALLOCATOR_TESTRUNNER_H

