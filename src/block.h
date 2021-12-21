#ifndef SRC_BLOCK_H_
#define SRC_BLOCK_H_

#include <linmath.h>
#include <stdbool.h>
#include <stdint.h>

typedef uint8_t BlockIndex;

typedef struct {
  bool transparent;
  const char *name;
} BlockDef;

const static BlockDef BLOCKS[] = {
    (BlockDef){.transparent = true, .name = "air"},
    (BlockDef){.transparent = false, .name = "stone"},
    (BlockDef){.transparent = false, .name = "grass"},
    (BlockDef){.transparent = false, .name = "soil"},
};

#define BLOCKS_LEN (sizeof(BLOCKS) / sizeof(BlockDef))

#define BLOCK_TEXTURE_SIZE 16

/// The height in pixels of the texture atlas
#define BLOCK_TEXTURE_ATLAS_HEIGHT (BLOCK_TEXTURE_SIZE * BLOCKS_LEN)

/// the width in pixels of the texture atlas
#define BLOCK_TEXTURE_ATLAS_WIDTH (BLOCK_TEXTURE_SIZE * 6)

/// the length in bytes of the texture atlas
#define BLOCK_TEXTURE_ATLAS_LEN                                                \
  (BLOCK_TEXTURE_ATLAS_WIDTH * BLOCK_TEXTURE_ATLAS_HEIGHT * 4)

// the size of the tile in normalized texture corodinates
#define BLOCK_TILE_TEX_XSIZE ((float)(1.0/6))
#define BLOCK_TILE_TEX_YSIZE (1.0f / ((float)sizeof(BLOCKS) / sizeof(BlockDef)))

typedef enum {
  Block_DOWN,
  Block_UP,
  Block_LEFT,
  Block_RIGHT,
  Block_BACK,
  Block_FRONT,
} BlockFaceKind;

void block_buildTextureAtlas(                       //
    uint8_t pTextureAtlas[BLOCK_TEXTURE_ATLAS_LEN], //
    const char *assetPath                           //
);

#endif // block_h_INCLUDED
