/*
 * Copyright 2019 Govind Pimpale
 * error_handle.h
 *
 *  Created on: Aug 7, 2018
 *      Author: gpi
 */

#ifndef SRC_ERRORS_H_
#define SRC_ERRORS_H_

#include <vulkan/vulkan.h>

#define UNUSED(x) (void)(x)
#define PANIC() exit(EXIT_FAILURE)

typedef enum ErrSeverity {
  ERR_LEVEL_DEBUG = 1,
  ERR_LEVEL_INFO = 2,
  ERR_LEVEL_WARN = 3,
  ERR_LEVEL_ERROR = 4,
  ERR_LEVEL_FATAL = 5,
  ERR_LEVEL_UNKNOWN = 6,
} ErrSeverity;

typedef enum ErrVal {
  ERR_OK = 0,
  ERR_UNKNOWN = 1,
  ERR_NOTSUPPORTED = 2,
  ERR_UNSAFE = 3,
  ERR_BADARGS = 4,
  ERR_OUTOFDATE = 5,
  ERR_ALLOCFAIL = 6,
  ERR_MEMORY = 7,
} ErrVal;

char *vkstrerror(VkResult err);
char *levelstrerror(ErrSeverity level);

void logError(ErrSeverity level, const char *message, ...);

#define LOG_ERROR(level, fmt, ...)                                             \
  do {                                                                            \
    char macro_message_formatted[MAX_PRINT_LENGTH];                                  \
    snprintf(macro_message_formatted, fmt, MAX_PRINT_LENGTH, __VA_ARGS__);           \
    printf("%s: %s: %s\n", APPNAME, levelstrerror(level), macro_message_formatted);  \
  } while(0)

#endif /* SRC_ERRORS_H_ */
