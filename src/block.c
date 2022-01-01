#include "block.h"

#include "errors.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <farbfeld.h>

// overwrites the texture atlas with bmp data
static void overwriteRectBmp(uint8_t *rgbaBuffer,     //
                             uint32_t bufferWidthPx,  //
                             uint32_t bufferHeightPx, //
                             uint32_t xoff,           //
                             uint32_t yoff,           //
                             const farbfeld_img *src  //
) {
  uint32_t xsize = src->xsize;
  uint32_t ysize = src->ysize;

  // prevent overflow
  assert(xoff + xsize <= bufferWidthPx);
  assert(yoff + ysize <= bufferHeightPx);

  for (uint32_t y = 0; y < ysize; y++) {
    uint32_t bufY = y + yoff;
    for (uint32_t x = 0; x < xsize; x++) {
      // calculate position in x buffer
      uint32_t bufX = x + xoff;

      // calculate pixel index in buffer
      uint32_t dstPxIndex = bufY * bufferWidthPx + bufX;
      uint32_t srcPxIndex = y * xsize + x;
      // set colors
      rgbaBuffer[dstPxIndex * 4 + 0] = src->data[srcPxIndex * 4 + 0] / 256;
      rgbaBuffer[dstPxIndex * 4 + 1] = src->data[srcPxIndex * 4 + 1] / 256;
      rgbaBuffer[dstPxIndex * 4 + 2] = src->data[srcPxIndex * 4 + 2] / 256;
      rgbaBuffer[dstPxIndex * 4 + 3] = src->data[srcPxIndex * 4 + 3] / 256;
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
    faceFilename = "down.ff";
    break;
  case Block_UP:
    faceFilename = "up.ff";
    break;
  case Block_LEFT:
    faceFilename = "left.ff";
    break;
  case Block_RIGHT:
    faceFilename = "right.ff";
    break;
  case Block_BACK:
    faceFilename = "back.ff";
    break;
  case Block_FRONT:
    faceFilename = "front.ff";
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

  farbfeld_img img;
  farbfeld_error e = read_farbfeld_img(&img, fileName);
  if (e != farbfeld_OK) {
    LOG_ERROR_ARGS(ERR_LEVEL_FATAL, "could not open farbfeld file at: %s", fileName);
    PANIC();
  }

  // assert dimensions of image
  if(img.xsize != BLOCK_TEXTURE_SIZE || img.ysize != BLOCK_TEXTURE_SIZE) {
    LOG_ERROR_ARGS(ERR_LEVEL_FATAL, "expected dimensions %u by %u: %s", BLOCK_TEXTURE_SIZE, BLOCK_TEXTURE_SIZE, fileName);
    PANIC();
  }

  free(fileName);

  // now write to area
  overwriteRectBmp(               //
      pTextureAtlas,              //
      BLOCK_TEXTURE_ATLAS_WIDTH,  //
      BLOCK_TEXTURE_ATLAS_HEIGHT, //
      face * BLOCK_TEXTURE_SIZE,  //
      index * BLOCK_TEXTURE_SIZE, //
      &img                        //
  );
  free_farbfeld_img(&img);
}

void block_buildTextureAtlas(                       //
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
