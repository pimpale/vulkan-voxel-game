#include <math.h>
#include <stdlib.h>

#include "block.h"
#include "world.h"

// gets the world chunk coordinate given from the centerLoc and the local array
// position (ax, ay, az)
static void arrayCoords_to_worldChunkCoords( //
    ivec3 dest,                              //
    const ivec3 centerLoc,                   //
    uint32_t ax,                             // array x
    uint32_t ay,                             // array y
    uint32_t az                              // array z
) {
  ivec3 localOffset = {(int32_t)ax - RENDER_DISTANCE_X / 2,
                       (int32_t)ay - RENDER_DISTANCE_Y / 2,
                       (int32_t)az - RENDER_DISTANCE_Z / 2};

  ivec3_add(dest, centerLoc, localOffset);
}

// converts from world chunk coordinates to global block coordinates
static void wld_worldChunkCoords_to_iBlockCoords( //
    ivec3 iBlockCoords,                           //
    const ivec3 worldChunkCoords                  //
) {
  iBlockCoords[0] = worldChunkCoords[0] * CHUNK_X_SIZE;
  iBlockCoords[1] = worldChunkCoords[1] * CHUNK_Y_SIZE;
  iBlockCoords[2] = worldChunkCoords[2] * CHUNK_Z_SIZE;
}

// converts from world chunk coordinates to global block coordinates
static void wld_worldChunkCoords_to_blockCoords( //
    vec3 blockCoords,                            //
    const ivec3 worldChunkCoords                 //
) {
  ivec3 tmp;
  wld_worldChunkCoords_to_iBlockCoords(tmp, worldChunkCoords);
  ivec3_to_vec3(blockCoords, tmp);
}

void wld_blockCoord_to_worldChunkCoords( //
    ivec3 chunkCoord,                    //
    const vec3 blockCoord                //
) {
  chunkCoord[0] = (int32_t)(blockCoord[0] / CHUNK_X_SIZE);
  chunkCoord[1] = (int32_t)(blockCoord[1] / CHUNK_Y_SIZE);
  chunkCoord[2] = (int32_t)(blockCoord[2] / CHUNK_Z_SIZE);
}

struct Chunk_s {
  uint32_t blocks[CHUNK_X_SIZE][CHUNK_Y_SIZE][CHUNK_Z_SIZE];
  // if a chunk has vertexCount == 0 then the Vulkan objects are undefined
  uint32_t vertexCount;

  // these 2 are only defined if vertexCount > 0
  VkBuffer vertexBuffer;
  VkDeviceMemory vertexBufferMemory;
};

static void blocks_count_vertexes_internal(                         //
    uint32_t *pVertexCount,                                         //
    const uint32_t blocks[CHUNK_X_SIZE][CHUNK_Y_SIZE][CHUNK_Z_SIZE] //
) {
  // first look through all blocks and count how many opaque we have
  uint32_t faceCount = 0;

  for (uint32_t x = 0; x < CHUNK_X_SIZE; x++) {
    for (uint32_t y = 0; y < CHUNK_Y_SIZE; y++) {
      for (uint32_t z = 0; z < CHUNK_Z_SIZE; z++) {
        // check that its not transparent
        if (BLOCKS[blocks[x][y][z]].transparent) {
          continue;
        }

        // left face
        if (x == 0 || BLOCKS[blocks[x - 1][y][z]].transparent) {
          faceCount++;
        }
        // right face
        if (x == CHUNK_X_SIZE - 1 || BLOCKS[blocks[x + 1][y][z]].transparent) {
          faceCount++;
        }

        // upper face
        if (y == 0 || BLOCKS[blocks[x][y - 1][z]].transparent) {
          faceCount++;
        }
        // lower face
        if (y == CHUNK_Y_SIZE - 1 || BLOCKS[blocks[x][y + 1][z]].transparent) {
          faceCount++;
        }

        // front face
        if (z == 0 || BLOCKS[blocks[x][y][z - 1]].transparent) {
          faceCount++;
        }
        // back face
        if (z == CHUNK_Z_SIZE - 1 || BLOCKS[blocks[x][y][z + 1]].transparent) {
          faceCount++;
        }
      }
    }
  }

  // now set answer
  *pVertexCount = faceCount * 6;
}

// returns the number of vertexes written
static uint32_t blocks_mesh_internal(                               //
    Vertex *pVertexes,                                              //
    const vec3 offset,                                              //
    const uint32_t blocks[CHUNK_X_SIZE][CHUNK_Y_SIZE][CHUNK_Z_SIZE] //
) {
  uint32_t i = 0;
  for (uint32_t x = 0; x < CHUNK_X_SIZE; x++) {
    for (uint32_t y = 0; y < CHUNK_Y_SIZE; y++) {
      for (uint32_t z = 0; z < CHUNK_Z_SIZE; z++) {
        // check that its not transparent
        if (BLOCKS[blocks[x][y][z]].transparent) {
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
        if (x == 0 || BLOCKS[blocks[x - 1][y][z]].transparent) {
          pVertexes[i++] = lbu;
          pVertexes[i++] = lfu;
          pVertexes[i++] = lbl;
          pVertexes[i++] = lbl;
          pVertexes[i++] = lfl;
          pVertexes[i++] = lfu;
        }
        // right face
        if (x == CHUNK_X_SIZE - 1 || BLOCKS[blocks[x + 1][y][z]].transparent) {
          pVertexes[i++] = rbu;
          pVertexes[i++] = rfu;
          pVertexes[i++] = rbl;
          pVertexes[i++] = rbl;
          pVertexes[i++] = rfl;
          pVertexes[i++] = rfu;
        }

        // upper face
        if (y == 0 || BLOCKS[blocks[x][y - 1][z]].transparent) {
          pVertexes[i++] = lbu;
          pVertexes[i++] = rbu;
          pVertexes[i++] = lfu;
          pVertexes[i++] = lfu;
          pVertexes[i++] = rfu;
          pVertexes[i++] = rbu;
        }
        // lower face
        if (y == CHUNK_Y_SIZE - 1 || BLOCKS[blocks[x][y + 1][z]].transparent) {
          pVertexes[i++] = lbl;
          pVertexes[i++] = rbl;
          pVertexes[i++] = lfl;
          pVertexes[i++] = lfl;
          pVertexes[i++] = rfl;
          pVertexes[i++] = rbl;
        }

        // back face
        if (z == 0 || BLOCKS[blocks[x][y][z - 1]].transparent) {
          pVertexes[i++] = lbu;
          pVertexes[i++] = rbu;
          pVertexes[i++] = lbl;
          pVertexes[i++] = lbl;
          pVertexes[i++] = rbl;
          pVertexes[i++] = rbu;
        }

        // front face
        if (z == CHUNK_Z_SIZE - 1 || BLOCKS[blocks[x][y][z + 1]].transparent) {
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

static void new_Chunk_internal(            //
    Chunk *c,                              //
    const struct osn_context *noiseCtx,    //
    ivec3 worldChunkCoords,                //
    const VkDevice device,                 //
    const VkPhysicalDevice physicalDevice, //
    const VkCommandPool commandPool,       //
    const VkQueue graphicsQueue            //
) {
  // convert into worldspace measured in blocks
  vec3 chunkOffset;
  wld_worldChunkCoords_to_blockCoords(chunkOffset, worldChunkCoords);

  // generate chunk data
  double scale = 20.0;
  for (uint32_t x = 0; x < CHUNK_X_SIZE; x++) {
    for (uint32_t y = 0; y < CHUNK_Y_SIZE; y++) {
      for (uint32_t z = 0; z < CHUNK_Z_SIZE; z++) {
        // calculate world coordinates in blocks
        double wx = x + (double)chunkOffset[0];
        double wy = y + (double)chunkOffset[1];
        double wz = z + (double)chunkOffset[2];

        double val =
            open_simplex_noise3(noiseCtx, wx / scale, wy / scale, wz / scale);

        if (val > 0.0) {
          c->blocks[x][y][z] = 1; // soil
        } else {
          c->blocks[x][y][z] = 0; // air
        }
      }
    }
  }

  // count chunk vertexes
  blocks_count_vertexes_internal(&c->vertexCount, c->blocks);

  if (c->vertexCount > 0) {
    // write mesh to vertex
    Vertex *vertexData = malloc(c->vertexCount * sizeof(Vertex));
    blocks_mesh_internal(vertexData, chunkOffset, c->blocks);

    // create vertex buffer + backing memory
    new_VertexBuffer(&c->vertexBuffer, &c->vertexBufferMemory, vertexData,
                     c->vertexCount, device, physicalDevice, commandPool,
                     graphicsQueue);

    free(vertexData);
  }
}

static void delete_Chunk_internal( //
    Chunk *c,                      //
    const VkDevice device          //
) {
  if(c->vertexCount > 0) {
    delete_Buffer(&c->vertexBuffer, device);
    delete_DeviceMemory(&c->vertexBufferMemory, device);
  }
}

void wld_new_WorldState(                   //
    WorldState *pWorldState,               //
    const ivec3 centerLoc,                 //
    const uint32_t seed,                   //
    const VkDevice device,                 //
    const VkPhysicalDevice physicalDevice, //
    const VkCommandPool commandPool,       //
    const VkQueue graphicsQueue            //
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
        ivec3 worldChunkCoords;
        arrayCoords_to_worldChunkCoords(worldChunkCoords, centerLoc, x, y, z);

        // allocate and set chunk
        Chunk *pThisChunk = malloc(sizeof(Chunk));

        // create the internal chunk vertex buffer
        new_Chunk_internal(      //
            pThisChunk,          //
            pWorldState->noise1, //
            worldChunkCoords,    //
            device,              //
            physicalDevice,      //
            commandPool,         //
            graphicsQueue        //
        );
        // set the chunk
        pWorldState->local[x][y][z] = pThisChunk;
      }
    }
  }
}

void wld_delete_WorldState(  //
    WorldState *pWorldState, //
    const VkDevice device    //
) {
  // free simplex noise
  open_simplex_noise_free(pWorldState->noise1);
  // free chunks
  for (uint32_t x = 0; x < RENDER_DISTANCE_X; x++) {
    for (uint32_t y = 0; y < RENDER_DISTANCE_Y; y++) {
      for (uint32_t z = 0; z < RENDER_DISTANCE_Z; z++) {
        // delete and free
        delete_Chunk_internal(pWorldState->local[x][y][z], device);
        free(pWorldState->local[x][y][z]);
      }
    }
  }
}

void wld_count_vertexBuffers(     //
    uint32_t *pVertexBufferCount, //
    const WorldState *pWorldState //
) {
  uint32_t count = 0;
  for (uint32_t x = 0; x < RENDER_DISTANCE_X; x++) {
    for (uint32_t y = 0; y < RENDER_DISTANCE_Y; y++) {
      for (uint32_t z = 0; z < RENDER_DISTANCE_Z; z++) {
        uint32_t vertexCount = pWorldState->local[x][y][z]->vertexCount;
        if (vertexCount > 0) {
          count++;
        }
      }
    }
  }
  *pVertexBufferCount = count;
}

// these buffers are for reading only! don't delete or modify
void wld_getVertexBuffers(        //
    VkBuffer *pVertexBuffers,     //
    uint32_t *pVertexCounts,      //
    const WorldState *pWorldState //
) {
  uint32_t i = 0;
  for (uint32_t x = 0; x < RENDER_DISTANCE_X; x++) {
    for (uint32_t y = 0; y < RENDER_DISTANCE_Y; y++) {
      for (uint32_t z = 0; z < RENDER_DISTANCE_Z; z++) {
        uint32_t vertexCount = pWorldState->local[x][y][z]->vertexCount;
        if (vertexCount > 0) {
          pVertexBuffers[i] = pWorldState->local[x][y][z]->vertexBuffer;
          pVertexCounts[i] = vertexCount;
          // increment index
          i++;
        }
      }
    }
  }
}

static int32_t max(int32_t a, int32_t b) {
  if (a > b) {
    return a;
  } else {
    return b;
  }
}

static int32_t min(int32_t a, int32_t b) {
  if (a < b) {
    return a;
  } else {
    return b;
  }
}

void wld_set_center(                       //
    WorldState *pWorldState,               //
    const ivec3 centerLoc,                 //
    const VkDevice device,                 //
    const VkPhysicalDevice physicalDevice, //
    const VkCommandPool commandPool,       //
    const VkQueue graphicsQueue            //
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
  ivec3_sub(disp, centerLoc, pWorldState->centerLoc);

  // now copy all the chunks we can from old to new

  // idea:
  //
  // old: *****
  // new:   *****
  //
  // disp = +2
  //
  // new[0] = old[2]
  // new[1] = old[3]
  // new[2] = old[4]
  //
  // new[0] = old[0+disp]
  // new[1] = old[1+disp]
  // new[2] = old[2+disp]
  //
  // start = max(0, 0-disp) = max(0, -2) = 0 (inclusive index)
  // end = min(len, len-disp) = min(5, 5-2) = 3 (exclusive index)

  int32_t startx = max(0, 0 - disp[0]);
  int32_t starty = max(0, 0 - disp[1]);
  int32_t startz = max(0, 0 - disp[2]);
  int32_t endx = min(RENDER_DISTANCE_X, RENDER_DISTANCE_X - disp[0]);
  int32_t endy = min(RENDER_DISTANCE_Y, RENDER_DISTANCE_Y - disp[1]);
  int32_t endz = min(RENDER_DISTANCE_Z, RENDER_DISTANCE_Z - disp[2]);

  for (int32_t x = startx; x < endx; x++) {
    for (int32_t y = starty; y < endy; y++) {
      for (int32_t z = startz; z < endz; z++) {
        // move some values from old to new array
        pWorldState->local[x][y][z] =
            old[x + disp[0]][y + disp[1]][z + disp[2]];

        // set to null
        old[x + disp[0]][y + disp[1]][z + disp[2]] = NULL;
      }
    }
  }

  // free all the chunks we weren't able to copy
  for (uint32_t x = 0; x < RENDER_DISTANCE_X; x++) {
    for (uint32_t y = 0; y < RENDER_DISTANCE_Y; y++) {
      for (uint32_t z = 0; z < RENDER_DISTANCE_Z; z++) {
        Chunk *outOfRangeChunk = old[x][y][z];
        if (outOfRangeChunk != NULL) {
          // delete and free
          delete_Chunk_internal(outOfRangeChunk, device);
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
          ivec3 worldChunkCoords;
          arrayCoords_to_worldChunkCoords(worldChunkCoords, centerLoc, x, y, z);

          // generate and set chunk
          Chunk *pThisChunk = malloc(sizeof(Chunk));
          new_Chunk_internal(      //
              pThisChunk,          //
              pWorldState->noise1, //
              worldChunkCoords,    //
              device,              //
              physicalDevice,      //
              commandPool,         //
              graphicsQueue        //

          );
          pWorldState->local[x][y][z] = pThisChunk;
        }
      }
    }
  }

  ivec3_dup(pWorldState->centerLoc, centerLoc);
}

bool wld_centered(                 //
    const WorldState *pWorldState, //
    const ivec3 blockCoord         //
) {
  return blockCoord[0] == pWorldState->centerLoc[0] &&
         blockCoord[1] == pWorldState->centerLoc[1] &&
         blockCoord[2] == pWorldState->centerLoc[2];
}
