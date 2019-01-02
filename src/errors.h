/*
 * error_handle.h
 *
 *  Created on: Aug 7, 2018
 *      Author: gpi
 */

#ifndef SRC_ERRORS_H_
#define SRC_ERRORS_H_

#define DEBUG 1
#define INFO 2
#define WARN 3
#define ERROR 4
#define FATAL 5
#define UNKNOWN 6

#define DEBUG_MSG "debug"
#define INFO_MSG "info"
#define WARN_MSG "warning"
#define ERROR_MSG "error"
#define FATAL_MSG "fatal"
#define UNKNOWN_MSG "unknown severity"

#define UNUSED(x) (void)(x)

#define ERR_OK 0
#define ERR_UNKNOWN 1
#define ERR_NOTSUPPORTED 2
#define ERR_UNSAFE 3
#define ERR_BADARGS 4
#define ERR_OUTOFDATE 5
#define ERR_ALLOCFAIL 6
#define ERR_MEMORY 7

typedef uint32_t ErrVal;

void errLog(int level, const char *message, ...);

void panic();

#endif /* SRC_ERRORS_H_ */
