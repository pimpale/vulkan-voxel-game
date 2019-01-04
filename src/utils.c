/*
 * util_methods.c
 *
 *  Created on: Sep 15, 2018
 *      Author: gpi
 */

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "constants.h"
#include "errors.h"
#include "utils.h"

uint64_t getLength(FILE *f) {
  int64_t currentpos = ftell(f);
  fseek(f, 0, SEEK_END);
  int64_t size = ftell(f);
  fseek(f, currentpos, SEEK_SET);
  return ((uint64_t)size);
}
/**
 * Mallocs
 */
void readShaderFile(const char *filename, uint32_t *length, uint32_t **code) {
  FILE *fp = fopen(filename, "rb");
  if (!fp) {
    errLog(FATAL, "could not read file\n");
    panic();
  }
  /* We can coerce to a 32 bit, because no realistic files will be
	 * greater than 2 GB */
  uint32_t filesize = (uint32_t) getLength(fp);
  uint32_t filesizepadded =
      (filesize % 4 == 0 ? filesize * 4 : (filesize + 1) * 4) / 4;

  char *str = malloc(filesizepadded);
  if (!str) {
    errLog(FATAL, "Could not read shader file: %s\n", strerror(errno));
    fclose(fp);
    panic();
    return;
  }

  fread(str, filesize, sizeof(char), fp);
  fclose(fp);

  /*pad data*/
  for (uint32_t i = filesize; i < filesizepadded; i++) {
    str[i] = 0;
  }

  /*set up*/
  *length = filesizepadded;
  *code = (uint32_t *) ((void*)str);
  return;
}
