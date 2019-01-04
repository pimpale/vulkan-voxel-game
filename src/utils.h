/*
 * util_methods.h
 *
 *  Created on: Sep 2, 2018
 *      Author: gpi
 */

#ifndef SRC_UTILS_H_
#define SRC_UTILS_H_

#include <stdint.h>
#include <stdio.h>

uint64_t getLength(FILE *f);

void findMatchingStrings(const char *const *ppData, uint32_t dataLen,
                         const char *const *ppQuery, uint32_t queryLen,
                         char **ppResult, uint32_t resultLen,
                         uint32_t *pMatches);

void readShaderFile(const char *filename, uint32_t *length, uint32_t **code);

#endif /* SRC_UTILS_H_ */
