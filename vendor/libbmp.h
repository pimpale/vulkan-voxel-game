#ifndef SRC_LIBBMP_H_
#define SRC_LIBBMP_H_

#include <stdint.h>
#include <stdio.h>

#define BMP_MAGIC 19778

#define BMP_GET_PADDING(a) ((a) % 4)

enum bmp_error {
  BMP_FILE_NOT_OPENED = -4,
  BMP_HEADER_NOT_INITIALIZED,
  BMP_INVALID_FILE,
  BMP_ERROR,
  BMP_OK = 0
};

typedef struct bmp_header_s {
  uint32_t bfSize;
  uint32_t bfReserved;
  uint32_t bfOffBits;

  uint32_t biSize;
  int32_t biWidth;
  int32_t biHeight;
  uint16_t biPlanes; // must be 1
  uint16_t biBitCount;
  uint32_t biCompression;
  uint32_t biSizeImage;
  int32_t biXPelsPerMeter;
  int32_t biYPelsPerMeter;
  uint32_t biClrUsed;
  uint32_t biClrImportant;
} bmp_header;

typedef struct bmp_pixel_s {
  uint8_t blue;
  uint8_t green;
  uint8_t red;
} bmp_pixel;

// This is faster than a function call
#define BMP_PIXEL(r, g, b) ((bmp_pixel){(b), (g), (r)})

typedef struct bmp_img_s {
  bmp_header img_header;
  bmp_pixel **img_pixels;
} bmp_img;

// BMP_HEADER
void bmp_header_init_df(bmp_header *, const int32_t, const int32_t);

enum bmp_error bmp_header_write(const bmp_header *, FILE *);

enum bmp_error bmp_header_read(bmp_header *, FILE *);

// BMP_PIXEL
void bmp_pixel_init(bmp_pixel *, const uint8_t, const uint8_t, const uint8_t);

// BMP_IMG
void bmp_img_alloc(bmp_img *);
void bmp_img_init_df(bmp_img *, const int32_t, const int32_t);
void bmp_img_free(bmp_img *);

enum bmp_error bmp_img_write(const bmp_img *, const char *);

enum bmp_error bmp_img_read(bmp_img *, const char *);

#endif /* SRC_LIBBMP_H_ */
