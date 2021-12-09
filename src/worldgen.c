#include "worldgen.h"
#include <stdlib.h>

static void chunk_count_vertexes( //
    uint32_t *pVertexCount,       //
    const Chunk *c                //
) {
  // first look through all blocks and count how many opaque we have
  uint32_t faceCount = 0;

  for (uint32_t x = 0; x < CHUNK_SIZE; x++) {
    for (uint32_t y = 0; y < CHUNK_SIZE; y++) {
      for (uint32_t z = 0; z < CHUNK_SIZE; z++) {
        // check that its not transparent
        if (c->blocks[x][y][z].transparent) {
          continue;
        }

        // left face
        if (x == 0 || c->blocks[x - 1][y][z].transparent) {
          faceCount++;
        }
        // right face
        if (x == CHUNK_SIZE - 1 || c->blocks[x + 1][y][z].transparent) {
          faceCount++;
        }

        // upper face
        if (y == 0 || c->blocks[x][y - 1][z].transparent) {
          faceCount++;
        }
        // lower face
        if (y == CHUNK_SIZE - 1 || c->blocks[x][y + 1][z].transparent) {
          faceCount++;
        }

        // front face
        if (z == 0 || c->blocks[x][y][z - 1].transparent) {
          faceCount++;
        }
        // back face
        if (z == CHUNK_SIZE - 1 || c->blocks[x][y][z + 1].transparent) {
          faceCount++;
        }
      }
    }
  }

  // now set answer
  *pVertexCount = faceCount * 6;
}

// returns the number of vertexes written
static uint32_t chunk_mesh( //
    Vertex *pVertexes,      //
    const vec3 offset,      //
    const Chunk *c          //
) {
  uint32_t i = 0;
  for (uint32_t x = 0; x < CHUNK_SIZE; x++) {
    for (uint32_t y = 0; y < CHUNK_SIZE; y++) {
      for (uint32_t z = 0; z < CHUNK_SIZE; z++) {
        // check that its not transparent
        if (c->blocks[x][y][z].transparent) {
          continue;
        }

        // calculate vertexes
        Vertex lbu = (Vertex){
            .position = {offset[0] + (float)x, offset[1] + (float)y,
                         offset[2] + (float)z},
            .color = {0.5f, 0.9f, 0.5f},
        };
        Vertex rbu = (Vertex){
            .position = {offset[0] + (float)(x + 1), offset[1] + (float)y,
                         offset[2] + (float)z},
            .color = {0.5f, 0.5f, 0.9f},
        };
        Vertex lfu = (Vertex){
            .position = {offset[0] + (float)x, offset[1] + (float)y,
                         offset[2] + (float)(z + 1)},
            .color = {0.9f, 0.5f, 0.5f},
        };
        Vertex rfu = (Vertex){
            .position = {offset[0] + (float)(x + 1), offset[1] + (float)y,
                         offset[2] + (float)(z + 1)},
            .color = {0.5f, 0.9f, 0.5f},
        };
        Vertex lbl = (Vertex){
            .position = {offset[0] + (float)x, offset[1] + (float)(y + 1),
                         offset[2] + (float)z},
            .color = {0.5f, 0.5f, 0.9f},
        };
        Vertex rbl = (Vertex){
            .position = {offset[0] + (float)(x + 1), offset[1] + (float)(y + 1),
                         offset[2] + (float)z},
            .color = {0.9f, 0.5f, 0.5f},
        };
        Vertex lfl = (Vertex){
            .position = {offset[0] + (float)x, offset[1] + (float)(y + 1),
                         offset[2] + (float)(z + 1)},
            .color = {0.5f, 0.5f, 0.5f},
        };
        Vertex rfl = (Vertex){
            .position = {offset[0] + (float)(x + 1), offset[1] + (float)(y + 1),
                         offset[2] + (float)(z + 1)},
            .color = {0.5f, 0.5f, 0.5f},
        };

        // left face
        if (x == 0 || c->blocks[x - 1][y][z].transparent) {
          pVertexes[i++] = lbu;
          pVertexes[i++] = lfu;
          pVertexes[i++] = lbl;
          pVertexes[i++] = lbl;
          pVertexes[i++] = lfl;
          pVertexes[i++] = lfu;
        }
        // right face
        if (x == CHUNK_SIZE - 1 || c->blocks[x + 1][y][z].transparent) {
          pVertexes[i++] = rbu;
          pVertexes[i++] = rfu;
          pVertexes[i++] = rbl;
          pVertexes[i++] = rbl;
          pVertexes[i++] = rfl;
          pVertexes[i++] = rfu;
        }

        // upper face
        if (y == 0 || c->blocks[x][y - 1][z].transparent) {
          pVertexes[i++] = lbu;
          pVertexes[i++] = rbu;
          pVertexes[i++] = lfu;
          pVertexes[i++] = lfu;
          pVertexes[i++] = rfu;
          pVertexes[i++] = rbu;
        }
        // lower face
        if (y == CHUNK_SIZE - 1 || c->blocks[x][y + 1][z].transparent) {
          pVertexes[i++] = lbl;
          pVertexes[i++] = rbl;
          pVertexes[i++] = lfl;
          pVertexes[i++] = lfl;
          pVertexes[i++] = rfl;
          pVertexes[i++] = rbl;
        }

        // back face
        if (z == 0 || c->blocks[x][y][z - 1].transparent) {
          pVertexes[i++] = lbu;
          pVertexes[i++] = rbu;
          pVertexes[i++] = lbl;
          pVertexes[i++] = lbl;
          pVertexes[i++] = rbl;
          pVertexes[i++] = rbu;
        }

        // front face
        if (z == CHUNK_SIZE - 1 || c->blocks[x][y][z + 1].transparent) {
          pVertexes[i++] = lfu;
          pVertexes[i++] = rfu;
          pVertexes[i++] = lfl;
          pVertexes[i++] = lfl;
          pVertexes[i++] = rfl;
          pVertexes[i++] = rfu;
        }
      }
    }
  }
  return i;
}

static void worldgen_chunk(Chunk *c, const struct osn_context *noiseCtx,
                    ivec3 chunkOffset) {
  double scale = 50.0;
  for (int32_t x = 0; x < CHUNK_SIZE; x++) {
    for (int32_t y = 0; y < CHUNK_SIZE; y++) {
      for (int32_t z = 0; z < CHUNK_SIZE; z++) {
        // calculate world coordinates in blocks
        int32_t wx = x + CHUNK_SIZE * chunkOffset.x;
        int32_t wy = y + CHUNK_SIZE * chunkOffset.y;
        int32_t wz = z + CHUNK_SIZE * chunkOffset.z;

        double val  = open_simplex_noise3(noiseCtx, wx / scale, wy / scale, wz / scale);

        if (val > 0.0) {
          c->blocks[x][y][z] = (Block){.transparent = false};
        } else {
          c->blocks[x][y][z] = (Block){.transparent = true};
        }
      }
    }
  }
}

void wg_new_WorldState_(     //
    WorldState *pWorldState, //
    const ivec3 centerLoc,   //
    const uint32_t seed      //
) {
  // initialize noise
  open_simplex_noise(seed, &pWorldState->noise1);
  // set center location
  pWorldState->centerLoc = centerLoc;

  // initialize chunks
  // note that, x, y, and z are relative to the array
  for (int32_t x = 0; x < RENDER_DISTANCE_X; x++) {
    for (int32_t y = 0; y < RENDER_DISTANCE_Y; y++) {
      for (int32_t z = 0; z < RENDER_DISTANCE_Z; z++) {
        // calculate chunk offset
        ivec3 chunkOffset = (ivec3){
            .x = centerLoc.x + (x - RENDER_DISTANCE_X / 2),
            .y = centerLoc.y + (y - RENDER_DISTANCE_Y / 2),
            .z = centerLoc.z + (z - RENDER_DISTANCE_Z / 2),
        };

        // generate and set chunk
        Chunk *pThisChunk = malloc(sizeof(Chunk));
        worldgen_chunk(pThisChunk, pWorldState->noise1, chunkOffset);
        pWorldState->local[x][y][z] = pThisChunk;
      }
    }
  }
}

void wg_world_count_vertexes(     //
    uint32_t *pVertexCount,       //
    const WorldState *pWorldState //
) {
  uint32_t sum = 0;
  for (int32_t x = 0; x < RENDER_DISTANCE_X; x++) {
    for (int32_t y = 0; y < RENDER_DISTANCE_Y; y++) {
      for (int32_t z = 0; z < RENDER_DISTANCE_Z; z++) {
        uint32_t chunkVertexes;
        chunk_count_vertexes(&chunkVertexes, pWorldState->local[x][y][z]);
        sum += chunkVertexes;
      }
    }
  }
  *pVertexCount = sum;
}

void wg_world_mesh(               //
    Vertex *pVertexes,            //
    const vec3 offset,            //
    const WorldState *pWorldState //
) {
  uint32_t i = 0;
  for (int32_t x = 0; x < RENDER_DISTANCE_X; x++) {
    for (int32_t y = 0; y < RENDER_DISTANCE_Y; y++) {
      for (int32_t z = 0; z < RENDER_DISTANCE_Z; z++) {
        // calculate the offset by adding the external offset to the block level
        // chunk offset

        // this is the integer chunk offset
        ivec3 chunkOffset = (ivec3){
            .x = pWorldState->centerLoc.x + (x - RENDER_DISTANCE_X / 2),
            .y = pWorldState->centerLoc.y + (y - RENDER_DISTANCE_Y / 2),
            .z = pWorldState->centerLoc.z + (z - RENDER_DISTANCE_Z / 2),
        };

        // convert into worldspace measured in blocks
        vec3 chunkOffsetInBlocks = {
            (float)chunkOffset.x * CHUNK_SIZE, //
            (float)chunkOffset.y * CHUNK_SIZE, //
            (float)chunkOffset.z * CHUNK_SIZE  //
        };

        // add the offset and chunkOffset together
        vec3 realOffset;
        vec3_add(realOffset, chunkOffsetInBlocks, offset);

        uint32_t meshed_vertexes =
            chunk_mesh(pVertexes + i, realOffset, pWorldState->local[x][y][z]);

        // we continue recording into the buffer at this location
        i += meshed_vertexes;
      }
    }
  }
}
