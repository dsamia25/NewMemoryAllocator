//
// Created by David Samia on 11/30/25.
//

#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <unistd.h>

// If logger not set, enable it by default.
#ifndef LOGGER
#define LOGGER 1
#endif

#ifndef LOG_LOG
#define LOG_LOG 1
#endif

#ifndef LOG_WARN
#define LOG_WARN 1
#endif

#ifndef LOG_ERROR
#define LOG_ERROR 1
#endif

// If logger color is not set, enable it by default.
#ifndef LOGGER_COLOR
#define LOGGER_COLOR 1
#endif

// If not set, auto adds a newline to each log message by default.
#ifndef AUTO_NEW_LINE
#define AUTO_NEW_LINE 1
#endif

#if AUTO_NEW_LINE
#define LINE_END_CHAR '\n'
#else
#define LINE_END_CHAR '\0'
#endif

/**
* Prints a simple, unformatted string like "hello world" without any %s's.
*/
#if LOGGER
#if LOGGER_COLOR
// Colorful log macros...
#define LOGGER_PATH_COLOR "\033[2;4;35m"
#define LOGGER_LOG_COLOR "\033[0;32m"
#define LOGGER_WARN_COLOR "\033[1;33m"
#define LOGGER_ERROR_COLOR "\033[1;31m"
#define LOGGER_COLOR_BLUE "\033[0;34m"
#define LOGGER_COLOR_RESET "\033[0m"

#define LOGGER_STATIC_OUTPUT(logTypeColor, logTypeString, str)\
  do {\
    if (isatty(STDERR_FILENO)) {\
      fprintf(stderr, "%s%s%s:%d:%s%s()%s:%s%s%s: %s%c", \
        LOGGER_PATH_COLOR, __FILE__, LOGGER_COLOR_RESET, \
        __LINE__, \
        LOGGER_COLOR_BLUE, __func__, LOGGER_COLOR_RESET, \
        logTypeColor, logTypeString, LOGGER_COLOR_RESET,\
        str, LINE_END_CHAR);\
      break;\
    }\
  } while(0);

#define LOGGER_DYNAMIC_OUTPUT(logTypeColor, logTypeString, fmt, ...)\
  do { \
    if (isatty(STDERR_FILENO)) {\
      fprintf(stderr, "%s%s%s:%d:%s%s()%s:%s%s%s: " fmt "%c", \
        LOGGER_PATH_COLOR, __FILE__, LOGGER_COLOR_RESET, \
        __LINE__, \
        LOGGER_COLOR_BLUE, __func__, LOGGER_COLOR_RESET, \
        logTypeColor, logTypeString, LOGGER_COLOR_RESET,\
        __VA_ARGS__, LINE_END_CHAR);\
      break;\
    }\
  } while (0);

#if LOG_LOG
#define LOGP(str) LOGGER_STATIC_OUTPUT(LOGGER_LOG_COLOR, "LOG", str)
#define LOG(fmt, ...) LOGGER_DYNAMIC_OUTPUT(LOGGER_LOG_COLOR, "LOG", fmt, __VA_ARGS__)
#endif

#if LOG_WARN
#define WARNP(str) LOGGER_STATIC_OUTPUT(LOGGER_WARN_COLOR, "WARN", str)
#define WARN(fmt, ...) LOGGER_DYNAMIC_OUTPUT(LOGGER_WARN_COLOR, "WARN", fmt, __VA_ARGS__)
#endif

#if LOG_ERROR
#define ERRORP(str) LOGGER_STATIC_OUTPUT(LOGGER_ERROR_COLOR, "ERROR", str)
#define ERROR(fmt, ...) LOGGER_DYNAMIC_OUTPUT(LOGGER_ERROR_COLOR, "ERROR", fmt, __VA_ARGS__)
#endif

#else
// Boring log macros...
#define LOGGER_STATIC_OUTPUT(logTypeString, str)\
  do {\
    if (isatty(STDERR_FILENO)) {\
      fprintf(stderr, "%s:%d:%s():%s: %s%c", \
        __FILE__, \
        __LINE__, \
        __func__, \
        logTypeString, \
        str, LINE_END_CHAR);\
      break;\
    }\
  } while(0);

#define LOGGER_DYNAMIC_OUTPUT(logTypeString, fmt, ...)\
  do { \
    if (isatty(STDERR_FILENO)) {\
      fprintf(stderr, "%s:%d:%s():%s: " fmt " %c", \
        __FILE__, \
        __LINE__, \
        __func__, \
        logTypeString, \
        __VA_ARGS__, LINE_END_CHAR);\
      break;\
    }\
  } while (0);

#if LOG_LOG
#define LOGP(str) LOGGER_STATIC_OUTPUT("LOG", str)
#define LOG(fmt, ...) LOGGER_DYNAMIC_OUTPUT("LOG", fmt, __VA_ARGS__)
#endif

#if LOG_WARN
#define WARNP(str) LOGGER_STATIC_OUTPUT("WARN", str)
#define WARN(fmt, ...) LOGGER_DYNAMIC_OUTPUT("WARN", fmt, __VA_ARGS__)
#endif

#if LOG_ERROR
#define ERRORP(str) LOGGER_STATIC_OUTPUT("ERROR", str)
#define ERROR(fmt, ...) LOGGER_DYNAMIC_OUTPUT("ERROR", fmt, __VA_ARGS__)
#endif

#endif

#endif

// Empty macros...
// The original version use the empty do/while loops but the single ';' works fine too.
// do {\
//   if((0)) {}\
// } while (0)
#ifndef LOGP
#define LOGP(str);
#endif
#ifndef LOG
#define LOG(fmt, ...);
#endif
#ifndef WARNP
#define WARNP(str);
#endif
#ifndef WARN
#define WARN(fmt, ...);
#endif
#ifndef ERRORP
#define ERRORP(str);
#endif
#ifndef ERROR
#define ERROR(fmt, ...);
#endif

#endif //LOGGER_H
