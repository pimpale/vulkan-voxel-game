#include <endian.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "farbfeld.h"

static inline bool read_be_uint32_t(FILE *file, uint32_t *ptr) {
  uint32_t be;
  // read 4 bytes out
  size_t bytes_read = fread(&be, 1, 4, file);
  if (bytes_read != 4) {
    return false;
  }
  *ptr = be32toh(be);
  return true;
}

farbfeld_error read_farbfeld_img( //
    farbfeld_img *img,            //
    const char *filename          //
) {
  FILE *file = fopen(filename, "rb");
  if (!file) {
    return farbfeld_FILE;
  }

  // check header
  char farbfeld_header[8];
  size_t header_bytes_read = fread(farbfeld_header, 1, 8, file);
  if (header_bytes_read != 8 || memcmp(farbfeld_header, "farbfeld", 8) != 0) {
    fclose(file);
    return farbfeld_INVALID;
  }

  // read height and width
  uint32_t xsize;
  uint32_t ysize;
  bool xsize_successful = read_be_uint32_t(file, &xsize);
  bool ysize_successful = read_be_uint32_t(file, &ysize);
  if (!xsize_successful || !ysize_successful) {
    fclose(file);
    return farbfeld_INVALID;
  }

  // size of the data array
  size_t datasize = xsize * ysize * sizeof(uint16_t) * 4;
  uint16_t *data = malloc(datasize);

  // read data at once
  size_t data_read_bytes = fread(data, 1, datasize, file);
  if (data_read_bytes != datasize) {
    free(data);
    fclose(file);
    return farbfeld_INVALID;
  }
  // we don't need the file any more
  fclose(file);

  // encode data in host order
  for (size_t i = 0; i < xsize * ysize * 4; i++) {
    data[i] = be16toh(data[i]);
  }

  // write struct
  img->xsize = xsize;
  img->ysize = ysize;
  img->data = data;

  return farbfeld_OK;
}

void free_farbfeld_img( //
    farbfeld_img *img   //
) {
  free(img->data);
}
