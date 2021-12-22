#ifndef farbfeld_h_INCLUDED
#define farbfeld_h_INCLUDED

#include <stdint.h>

typedef struct {
  uint32_t xsize;
  uint32_t ysize;
  uint16_t *data;
} farbfeld_img;

typedef enum { farbfeld_OK, farbfeld_FILE, farbfeld_INVALID } farbfeld_error;

farbfeld_error read_farbfeld_img( //
    farbfeld_img *img,            //
    const char *filename          //
);

void free_farbfeld_img( //
    farbfeld_img *img   //
);

#endif // farbfeld_h_INCLUDED
