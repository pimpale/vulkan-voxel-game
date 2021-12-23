#include <endian.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "farbfeld.h"

farbfeld_error read_farbfeld_img( //
    farbfeld_img *img,            //
    const char *filename          //
) {
  FILE *file = fopen(filename, "rb");
  if (!file) {
    return farbfeld_FILE;
  }

  // check header
  char farbfeld_header[16];
  size_t header_bytes_read = fread(farbfeld_header, 1, 16, file);
  if (header_bytes_read != 16 || memcmp(farbfeld_header, "farbfeld", 8) != 0) {
    fclose(file);
    return farbfeld_INVALID;
  }

  // read height and width
  uint32_t xsize = be32toh(*(uint32_t *)(farbfeld_header + 8));
  uint32_t ysize = be32toh(*(uint32_t *)(farbfeld_header + 12));

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
