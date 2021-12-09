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
  double scale = 20.0;
  for (int32_t x = 0; x < CHUNK_SIZE; x++) {
    for (int32_t y = 0; y < CHUNK_SIZE; y++) {
      for (int32_t z = 0; z < CHUNK_SIZE; z++) {
        // calculate world coordinates in blocks
        int32_t wx = x + CHUNK_SIZE * chunkOffset[0];
        int32_t wy = y + CHUNK_SIZE * chunkOffset[1];
        int32_t wz = z + CHUNK_SIZE * chunkOffset[2];

        double val =
            open_simplex_noise3(noiseCtx, wx / scale, wy / scale, wz / scale);

        if (val > 0.0) {
          c->blocks[x][y][z] = (Block){.transparent = false};
        } else {
          c->blocks[x][y][z] = (Block){.transparent = true};
        }
      }
    }
  }
}

// gets the chunk coordinate given from the centerLoc and the local array
// position (ax, ay, az)
void getChunkCoordinate(   //
    ivec3 dest,            //
    const ivec3 centerLoc, //
    uint32_t ax,           // array x
    uint32_t ay,           // array y
    uint32_t az            // array z
) {
  ivec3 localOffset = {(int32_t)ax - RENDER_DISTANCE_X / 2,
                       (int32_t)ay - RENDER_DISTANCE_Y / 2,
                       (int32_t)az - RENDER_DISTANCE_Z / 2};

  ivec3_add(dest, centerLoc, localOffset);
}

void wg_new_WorldState(      //
    WorldState *pWorldState, //
    const ivec3 centerLoc,   //
    const uint32_t seed      //
) {
  // initialize noise
  open_simplex_noise(seed, &pWorldState->noise1);
  // set center location
  ivec3_dup(pWorldState->centerLoc, centerLoc);

  // initialize chunks
  // note that, x, y, and z are relative to the array
  for (uint32_t x = 0; x < RENDER_DISTANCE_X; x++) {
    for (uint32_t y = 0; y < RENDER_DISTANCE_Y; y++) {
      for (uint32_t z = 0; z < RENDER_DISTANCE_Z; z++) {
        // calculate chunk offset
        ivec3 chunkOffset;
        getChunkCoordinate(chunkOffset, centerLoc, x, y, z);

        // generate and set chunk
        Chunk *pThisChunk = malloc(sizeof(Chunk));
        worldgen_chunk(pThisChunk, pWorldState->noise1, chunkOffset);
        pWorldState->local[x][y][z] = pThisChunk;
      }
    }
  }
}

void wg_delete_WorldState(  //
    WorldState *pWorldState //
) {
  open_simplex_noise_free(pWorldState->noise1);
  for (uint32_t x = 0; x < RENDER_DISTANCE_X; x++) {
    for (uint32_t y = 0; y < RENDER_DISTANCE_Y; y++) {
      for (uint32_t z = 0; z < RENDER_DISTANCE_Z; z++) {
        free(pWorldState->local[x][y][z]);
      }
    }
  }
}

void wg_world_count_vertexes(     //
    uint32_t *pVertexCount,       //
    const WorldState *pWorldState //
) {
  uint32_t sum = 0;
  for (uint32_t x = 0; x < RENDER_DISTANCE_X; x++) {
    for (uint32_t y = 0; y < RENDER_DISTANCE_Y; y++) {
      for (uint32_t z = 0; z < RENDER_DISTANCE_Z; z++) {
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
    const WorldState *pWorldState //
) {
  uint32_t i = 0;
  for (uint32_t x = 0; x < RENDER_DISTANCE_X; x++) {
    for (uint32_t y = 0; y < RENDER_DISTANCE_Y; y++) {
      for (uint32_t z = 0; z < RENDER_DISTANCE_Z; z++) {
        // this is the integer chunk offset
        ivec3 chunkOffset;
        getChunkCoordinate(chunkOffset, pWorldState->centerLoc, x, y, z);

        // convert into worldspace measured in blocks
        vec3 chunkOffsetInBlocks;
        ivec3_conv(chunkOffsetInBlocks, chunkOffset);
        vec3_scale(chunkOffsetInBlocks, chunkOffsetInBlocks, CHUNK_SIZE);

        uint32_t meshed_vertexes = chunk_mesh(
            pVertexes + i, chunkOffsetInBlocks, pWorldState->local[x][y][z]);

        // we continue recording into the buffer at this location
        i += meshed_vertexes;
      }
    }
  }
}

void wg_set_center(          //
    WorldState *pWorldState, //
    const ivec3 centerLoc    //
) {

  // copy our source volume to another array to avoid clobbering it
  Chunk *old[RENDER_DISTANCE_X][RENDER_DISTANCE_Y][RENDER_DISTANCE_Z];
  for (uint32_t x = 0; x < RENDER_DISTANCE_X; x++) {
    for (uint32_t y = 0; y < RENDER_DISTANCE_Y; y++) {
      for (uint32_t z = 0; z < RENDER_DISTANCE_Z; z++) {
        // move
        old[x][y][z] = pWorldState->local[x][y][z];
        pWorldState->local[x][y][z] = NULL;
      }
    }
  }

  // calculate our displacement from old state to new state
  ivec3 disp;
  ivec3_sub(disp, pWorldState->centerLoc, centerLoc);

  // now copy all the chunks we can from old to new
  for (int32_t x = 0; x < RENDER_DISTANCE_X - disp[0]; x++) {
    for (int32_t y = 0; y < RENDER_DISTANCE_Y - disp[1]; y++) {
      for (int32_t z = 0; z < RENDER_DISTANCE_Z - disp[2]; z++) {
        // move some values from old to new array
        pWorldState->local[x + disp[0]][y + disp[1]][z + disp[2]] =
            old[x][y][z];
        old[x][y][z] = NULL;
      }
    }
  }

  // free all the chunks we weren't able to copy
  for (uint32_t x = 0; x < RENDER_DISTANCE_X; x++) {
    for (uint32_t y = 0; y < RENDER_DISTANCE_Y; y++) {
      for (uint32_t z = 0; z < RENDER_DISTANCE_Z; z++) {
        Chunk *outOfRangeChunk = old[x][y][z];
        if (outOfRangeChunk != NULL) {
          free(outOfRangeChunk);
        }
      }
    }
  }

  // generate all chunks not yet initialized
  for (uint32_t x = 0; x < RENDER_DISTANCE_X; x++) {
    for (uint32_t y = 0; y < RENDER_DISTANCE_Y; y++) {
      for (uint32_t z = 0; z < RENDER_DISTANCE_Z; z++) {
        if (pWorldState->local[x][y][z] == NULL) {
          // calculate chunk offset
          ivec3 chunkOffset;
          getChunkCoordinate(chunkOffset, centerLoc, x, y, z);

          // generate and set chunk
          Chunk *pThisChunk = malloc(sizeof(Chunk));
          worldgen_chunk(pThisChunk, pWorldState->noise1, chunkOffset);
          pWorldState->local[x][y][z] = pThisChunk;
        }
      }
    }
  }
}
