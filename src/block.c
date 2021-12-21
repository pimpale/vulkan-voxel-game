#include "block.h"

#include "errors.h"

#include <libbmp.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

// overwrites the texture atlas with bmp data
static void overwriteRectBmp(uint8_t *rgbaBuffer,     //
                             uint32_t bufferWidthPx,  //
                             uint32_t bufferHeightPx, //
                             uint32_t xoff,           //
                             uint32_t yoff,           //
                             const bmp_img *src       //
) {
  uint32_t xsize = (uint32_t)abs(src->img_header.biWidth);
  uint32_t ysize = (uint32_t)abs(src->img_header.biHeight);

  // prevent overflow
  assert(xoff + xsize < bufferWidthPx);
  assert(yoff + ysize < bufferHeightPx);

  for (uint32_t y = 0; y < ysize; y++) {
    uint32_t bufY = y + yoff;
    for (uint32_t x = 0; x < xsize; x++) {
      // calculate position in x buffer
      uint32_t bufX = x + xoff;

      // calculate pixel index in buffer
      uint32_t pxIndex = bufY * bufferWidthPx + bufX;
      // set colors
      rgbaBuffer[pxIndex * 4 + 0] = src->img_pixels[y][x].red;
      rgbaBuffer[pxIndex * 4 + 1] = src->img_pixels[y][x].green;
      rgbaBuffer[pxIndex * 4 + 2] = src->img_pixels[y][x].blue;
      rgbaBuffer[pxIndex * 4 + 3] = 0xFF;
    }
  }
}

static void writePicTexAtlas(                       //
    uint8_t pTextureAtlas[BLOCK_TEXTURE_ATLAS_LEN], //
    BlockIndex index,                               //
    BlockFaceKind face,                             //
    const char *assetPath                           //
) {

  const char *blockName = BLOCKS[index].name;

  const char *faceFilename;
  switch (face) {
  case Block_DOWN:
    faceFilename = "down.bmp";
    break;
  case Block_UP:
    faceFilename = "up.bmp";
    break;
  case Block_LEFT:
    faceFilename = "left.bmp";
    break;
  case Block_RIGHT:
    faceFilename = "right.bmp";
    break;
  case Block_BACK:
    faceFilename = "back.bmp";
    break;
  case Block_FRONT:
    faceFilename = "front.bmp";
    break;
  }

  // get size of total buffer
  size_t fileNameSize = strlen(assetPath) + strlen("/") + strlen(blockName) +
                        strlen("/") + strlen(faceFilename) + 1;
  char *fileName = malloc(fileNameSize * sizeof(char));

  // build string
  strcpy(fileName, assetPath);
  strcat(fileName, "/");
  strcat(fileName, blockName);
  strcat(fileName, "/");
  strcat(fileName, faceFilename);

  bmp_img img;
  enum bmp_error e = bmp_img_read(&img, fileName);
  if (e != BMP_OK) {
    LOG_ERROR_ARGS(ERR_LEVEL_FATAL, "could not open bmp file at %s", fileName);
    PANIC();
  }
  free(fileName);

  // assert dimensions of image
  assert(img.img_header.biWidth == BLOCK_TEXTURE_SIZE);
  assert(img.img_header.biHeight == BLOCK_TEXTURE_SIZE);

  // now write to area
  overwriteRectBmp(               //
      pTextureAtlas,              //
      BLOCK_TEXUTRE_ATLAS_WIDTH,  //
      BLOCK_TEXUTRE_ATLAS_HEIGHT, //
      face * BLOCK_TEXTURE_SIZE,  //
      index * BLOCK_TEXTURE_SIZE, //
      &img                        //
  );
}

void block_buildTextureAtlas(                             //
    uint8_t pTextureAtlas[BLOCK_TEXTURE_ATLAS_LEN], //
    const char *assetPath                           //
) {
  for (BlockIndex i = 0; i < BLOCKS_LEN; i++) {
    // don't need to get texture for transparent blocks
    if (BLOCKS[i].transparent) {
      continue;
    }

    // write all six faces
    writePicTexAtlas(pTextureAtlas, i, Block_DOWN, assetPath);
    writePicTexAtlas(pTextureAtlas, i, Block_UP, assetPath);
    writePicTexAtlas(pTextureAtlas, i, Block_LEFT, assetPath);
    writePicTexAtlas(pTextureAtlas, i, Block_RIGHT, assetPath);
    writePicTexAtlas(pTextureAtlas, i, Block_BACK, assetPath);
    writePicTexAtlas(pTextureAtlas, i, Block_FRONT, assetPath);
  }
}
