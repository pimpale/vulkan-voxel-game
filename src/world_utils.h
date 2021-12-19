#ifndef WORLD_CONSTANTS_H
#define WORLD_CONSTANTS_H

#include <ivec3.h>
#include <linmath.h>

#include "block.h"

// Size of chunk in blocks
#define CHUNK_X_SIZE 32
#define CHUNK_Y_SIZE 32
#define CHUNK_Z_SIZE 32

// contains block data for the chunk
typedef struct {
  BlockIndex blocks[CHUNK_X_SIZE][CHUNK_Y_SIZE][CHUNK_Z_SIZE];
} ChunkData;

// converts from world chunk coordinates to global block coordinates
static inline void worldChunkCoords_to_iBlockCoords( //
    ivec3 iBlockCoords,                              //
    const ivec3 worldChunkCoords                     //
) {
  iBlockCoords[0] = worldChunkCoords[0] * CHUNK_X_SIZE;
  iBlockCoords[1] = worldChunkCoords[1] * CHUNK_Y_SIZE;
  iBlockCoords[2] = worldChunkCoords[2] * CHUNK_Z_SIZE;
}

static void blockCoords_to_worldChunkCoords( //
    ivec3 chunkCoord,                 //
    const vec3 blockCoord             //
) {
  chunkCoord[0] = (int32_t)(blockCoord[0] / CHUNK_X_SIZE);
  chunkCoord[1] = (int32_t)(blockCoord[1] / CHUNK_Y_SIZE);
  chunkCoord[2] = (int32_t)(blockCoord[2] / CHUNK_Z_SIZE);
}

// converts from world chunk coordinates to global block coordinates
static void worldChunkCoords_to_blockCoords( //
    vec3 blockCoords,                            //
    const ivec3 worldChunkCoords                 //
) {
  ivec3 tmp;
  worldChunkCoords_to_iBlockCoords(tmp, worldChunkCoords);
  ivec3_to_vec3(blockCoords, tmp);
}


#endif
