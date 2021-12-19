#include <assert.h>
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

static uint32_t blocks_count_vertexes_internal( //
    const ChunkData *pCd                        //
) {
  // first look through all blocks and count how many opaque we have
  uint32_t faceCount = 0;

  for (uint32_t x = 0; x < CHUNK_X_SIZE; x++) {
    for (uint32_t y = 0; y < CHUNK_Y_SIZE; y++) {
      for (uint32_t z = 0; z < CHUNK_Z_SIZE; z++) {
        // check that its not transparent
        if (BLOCKS[pCd->blocks[x][y][z]].transparent) {
          continue;
        }

        // left face
        if (x == 0 || BLOCKS[pCd->blocks[x - 1][y][z]].transparent) {
          faceCount++;
        }
        // right face
        if (x == CHUNK_X_SIZE - 1 ||
            BLOCKS[pCd->blocks[x + 1][y][z]].transparent) {
          faceCount++;
        }

        // upper face
        if (y == 0 || BLOCKS[pCd->blocks[x][y - 1][z]].transparent) {
          faceCount++;
        }
        // lower face
        if (y == CHUNK_Y_SIZE - 1 ||
            BLOCKS[pCd->blocks[x][y + 1][z]].transparent) {
          faceCount++;
        }

        // front face
        if (z == 0 || BLOCKS[pCd->blocks[x][y][z - 1]].transparent) {
          faceCount++;
        }
        // back face
        if (z == CHUNK_Z_SIZE - 1 ||
            BLOCKS[pCd->blocks[x][y][z + 1]].transparent) {
          faceCount++;
        }
      }
    }
  }

  // now set answer
  return faceCount * 6;
}

// returns the number of vertexes written
static uint32_t blocks_mesh_internal( //
    Vertex *pVertexes,                //
    const vec3 offset,                //
    const ChunkData *pCd              //
) {
  uint32_t i = 0;
  for (uint32_t x = 0; x < CHUNK_X_SIZE; x++) {
    for (uint32_t y = 0; y < CHUNK_Y_SIZE; y++) {
      for (uint32_t z = 0; z < CHUNK_Z_SIZE; z++) {
        // check that its not transparent
        if (BLOCKS[pCd->blocks[x][y][z]].transparent) {
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
        if (x == 0 || BLOCKS[pCd->blocks[x - 1][y][z]].transparent) {
          pVertexes[i++] = lbu;
          pVertexes[i++] = lfu;
          pVertexes[i++] = lbl;
          pVertexes[i++] = lbl;
          pVertexes[i++] = lfl;
          pVertexes[i++] = lfu;
        }
        // right face
        if (x == CHUNK_X_SIZE - 1 ||
            BLOCKS[pCd->blocks[x + 1][y][z]].transparent) {
          pVertexes[i++] = rbu;
          pVertexes[i++] = rfu;
          pVertexes[i++] = rbl;
          pVertexes[i++] = rbl;
          pVertexes[i++] = rfl;
          pVertexes[i++] = rfu;
        }

        // upper face
        if (y == 0 || BLOCKS[pCd->blocks[x][y - 1][z]].transparent) {
          pVertexes[i++] = lbu;
          pVertexes[i++] = rbu;
          pVertexes[i++] = lfu;
          pVertexes[i++] = lfu;
          pVertexes[i++] = rfu;
          pVertexes[i++] = rbu;
        }
        // lower face
        if (y == CHUNK_Y_SIZE - 1 ||
            BLOCKS[pCd->blocks[x][y + 1][z]].transparent) {
          pVertexes[i++] = lbl;
          pVertexes[i++] = rbl;
          pVertexes[i++] = lfl;
          pVertexes[i++] = lfl;
          pVertexes[i++] = rfl;
          pVertexes[i++] = rbl;
        }

        // back face
        if (z == 0 || BLOCKS[pCd->blocks[x][y][z - 1]].transparent) {
          pVertexes[i++] = lbu;
          pVertexes[i++] = rbu;
          pVertexes[i++] = lbl;
          pVertexes[i++] = lbl;
          pVertexes[i++] = rbl;
          pVertexes[i++] = rbu;
        }

        // front face
        if (z == CHUNK_Z_SIZE - 1 ||
            BLOCKS[pCd->blocks[x][y][z + 1]].transparent) {
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

// generate chunk data
static void blocks_gen(                //
    ChunkData *pCd,                    //
    const vec3 chunkOffset,            //
    const struct osn_context *noiseCtx //
) {
  double scale1 = 20.0;
  double scale2 = 10.0;
  for (uint32_t x = 0; x < CHUNK_X_SIZE; x++) {
    for (uint32_t y = 0; y < CHUNK_Y_SIZE; y++) {
      for (uint32_t z = 0; z < CHUNK_Z_SIZE; z++) {
        // calculate world coordinates in blocks
        double wx = x + (double)chunkOffset[0];
        double wy = y + (double)chunkOffset[1];
        double wz = z + (double)chunkOffset[2];
        double val = open_simplex_noise3(noiseCtx, wx / scale1, wy / scale1,
                                         wz / scale1);
        open_simplex_noise3(noiseCtx, wx / scale2, wy / scale2, wz / scale2);
        if (val > 0.0) {
          pCd->blocks[x][y][z] = 1; // soil
        } else {
          pCd->blocks[x][y][z] = 0; // air
        }
      }
    }
  }
}

struct ChunkGeometry_s {
  uint32_t vertexCount;
  // these 2 are only defined if vertexCount > 0
  VkBuffer vertexBuffer;
  VkDeviceMemory vertexBufferMemory;
};

static void new_ChunkGeometry(             //
    ChunkGeometry *c,                      //
    const ChunkData *data,                 //
    const vec3 chunkOffset,                //
    const VkDevice device,                 //
    const VkPhysicalDevice physicalDevice, //
    const VkCommandPool commandPool,       //
    const VkQueue queue                    //
) {
  // count chunk vertexes
  c->vertexCount = blocks_count_vertexes_internal(data);
  if (c->vertexCount > 0) {
    // write mesh to vertex
    Vertex *vertexData = malloc(c->vertexCount * sizeof(Vertex));
    blocks_mesh_internal(vertexData, chunkOffset, data);
    new_VertexBuffer(&c->vertexBuffer, &c->vertexBufferMemory, vertexData,
                     c->vertexCount, device, physicalDevice, commandPool,
                     queue);
    free(vertexData);
  }
}

static void delete_ChunkGeometry(ChunkGeometry *geometry,
                                 const VkDevice device) {
  delete_Buffer(&geometry->vertexBuffer, device);
  delete_DeviceMemory(&geometry->vertexBufferMemory, device);
}

typedef struct {
  ivec3 chunkCoord;
  ChunkData data;
  ChunkGeometry *pGeometry;
} ivec3_Chunk_KVPair;

static int ivec3_Chunk_KVPair_compare(const void *a, const void *b,
                                      UNUSED void *udata) {
  const ivec3_Chunk_KVPair *pa = a;
  const ivec3_Chunk_KVPair *pb = b;

  int32_t d0 = pa->chunkCoord[0] - pb->chunkCoord[0];
  int32_t d1 = pa->chunkCoord[1] - pb->chunkCoord[1];
  int32_t d2 = pa->chunkCoord[2] - pb->chunkCoord[2];

  if (d0 != 0) {
    return d0;
  }
  if (d1 != 0) {
    return d1;
  }
  return d2;
}

static uint64_t ivec3_Chunk_KVPair_hash(const void *item, uint64_t seed0,
                                        uint64_t seed1) {
  const ivec3_Chunk_KVPair *pair = item;
  return hashmap_sip(pair->chunkCoord, sizeof(ivec3), seed0, seed1);
}

void wld_new_WorldState(                  //
    WorldState *pWorldState,              //
    const ivec3 centerLoc,                //
    const uint32_t seed,                  //
    const VkQueue queue,                  //
    const VkCommandPool commandPool,      //
    const VkDevice device,                //
    const VkPhysicalDevice physicalDevice //
) {
  // set center location
  ivec3_dup(pWorldState->centerLoc, centerLoc);

  // set noise
  open_simplex_noise(seed, &pWorldState->noise1);

  // copy vulkan
  pWorldState->device = device;
  pWorldState->physicalDevice = physicalDevice;
  pWorldState->queue = queue;
  pWorldState->commandPool = commandPool;

  // initialize stacks to empty
  new_ivec3_vec(&pWorldState->togenerate);
  new_ivec3_vec(&pWorldState->tomesh);
  new_ivec3_vec(&pWorldState->ready);
  new_ivec3_vec(&pWorldState->tounload);

  // initialize hash maps
  pWorldState->chunk_map =
      hashmap_new(sizeof(ivec3_Chunk_KVPair), 0, 0, 0, ivec3_Chunk_KVPair_hash,
                  ivec3_Chunk_KVPair_compare, NULL, NULL);

  // initialize all of our neighboring chunks to be on the load list
  for (uint32_t x = 0; x < RENDER_DISTANCE_X; x++) {
    for (uint32_t y = 0; y < RENDER_DISTANCE_Y; y++) {
      for (uint32_t z = 0; z < RENDER_DISTANCE_Z; z++) {
        // push the chunk into our vector
        ivec3 tmp;
        arrayCoords_to_worldChunkCoords(tmp, centerLoc, x, y, z);
        ivec3_vec_push(pWorldState->togenerate, tmp);
      }
    }
  }
}

static bool wld_shouldBeLoaded(    //
    const WorldState *pWorldState, //
    const ivec3 worldChunkCoords   //
) {

  ivec3 minDisp = {-RENDER_DISTANCE_X / 2, -RENDER_DISTANCE_Y / 2,
                   -RENDER_DISTANCE_Z / 2};

  ivec3 maxDisp = {(RENDER_DISTANCE_X + 1) / 2, (RENDER_DISTANCE_Y + 1) / 2,
                   (RENDER_DISTANCE_Z + 1) / 2};

  ivec3 disp;
  ivec3_sub(disp, worldChunkCoords, pWorldState->centerLoc);

  return (disp[0] >= minDisp[0] && disp[0] <= maxDisp[0]) &&
         (disp[1] >= minDisp[1] && disp[1] <= maxDisp[1]) &&
         (disp[2] >= minDisp[2] && disp[2] <= maxDisp[2]);
}

static void wld_pushGarbage(WorldState *pWorldState, ChunkGeometry *geometry) {
  if (pWorldState->garbage_len >= pWorldState->garbage_cap) {
    pWorldState->garbage_cap *= 2;
    pWorldState->garbage_data =
        realloc(pWorldState->garbage_data,
                pWorldState->garbage_cap * sizeof(ChunkGeometry *));
  }

  pWorldState->garbage_data[pWorldState->garbage_len] = geometry;
  pWorldState->garbage_len++;
}

void wld_clearGarbage(WorldState *pWorldState) {
  for (uint32_t i = 0; i < pWorldState->garbage_len; i++) {
    delete_ChunkGeometry(pWorldState->garbage_data[i], pWorldState->device);
    free(pWorldState->garbage_data[i]);
  }
  pWorldState->garbage_len = 0;
}

void wld_update(            //
    WorldState *pWorldState //
) {
  // process stuff off the togenerate list
  for (uint32_t i = 0;
       i < MAX_CHUNKS_TO_GENERATE && ivec3_vec_len(pWorldState->togenerate) > 0;
       i++) {
    ivec3_Chunk_KVPair chunkToLoad;
    ivec3_vec_pop(pWorldState->togenerate, chunkToLoad.chunkCoord);

    // check that we still even need to load this
    if (!wld_shouldBeLoaded(pWorldState, chunkToLoad.chunkCoord)) {
      continue;
    }

    // check we haven't already loaded this
    if (hashmap_get(pWorldState->chunk_map, &chunkToLoad) != NULL) {
      continue;
    }

    // generate chunk, we need to give it the block coordinate to generate at
    vec3 chunkOffset;
    worldChunkCoords_to_blockCoords(chunkOffset, chunkToLoad.chunkCoord);
    blocks_gen(&chunkToLoad.data, chunkOffset, pWorldState->noise1);

    // no geometry yet
    chunkToLoad.pGeometry = NULL;

    // hashmap will clone the chunk to load
    hashmap_set(pWorldState->chunk_map, &chunkToLoad);

    // push the chunk coord to the chunks to mesh
    ivec3_vec_push(pWorldState->tomesh, chunkToLoad.chunkCoord);
  }

  // process stuff on the to mesh list
  for (uint32_t i = 0;
       i < MAX_CHUNKS_TO_MESH && ivec3_vec_len(pWorldState->tomesh) > 0; i++) {
    ivec3_Chunk_KVPair chunkToMesh;
    ivec3_vec_pop(pWorldState->tomesh, chunkToMesh.chunkCoord);

    ivec3_Chunk_KVPair *pChunk =
        hashmap_get(pWorldState->chunk_map, chunkToMesh.chunkCoord);

    if (pChunk->pGeometry != NULL) {
      // if some data already exists, place this geometry in the Garbage heap,
      // and make a new one
      wld_pushGarbage(pWorldState, pChunk->pGeometry);
      pChunk->pGeometry = malloc(sizeof(ChunkGeometry));
    } else {
      // otherwise malloc space
      pChunk->pGeometry = malloc(sizeof(ChunkGeometry));
    }

    vec3 chunkOffset;
    worldChunkCoords_to_blockCoords(chunkOffset, pChunk->chunkCoord);

    new_ChunkGeometry(pChunk->pGeometry, &pChunk->data, chunkOffset,
                      pWorldState->device, pWorldState->physicalDevice,
                      pWorldState->commandPool, pWorldState->queue);

    // push onto the ready list
    // push the chunk coord to the chunks to mesh
    ivec3_vec_push(pWorldState->ready, pChunk->chunkCoord);
  }

  // process stuff on the ready list
  for (int32_t i = (int32_t)ivec3_vec_len(pWorldState->ready) - 1; i >= 0;
       i--) {
    ivec3 chunkCoords;
    ivec3_vec_get(pWorldState->ready, (uint32_t)i, chunkCoords);

    // check that we still even need to have this
    if (!wld_shouldBeLoaded(pWorldState, chunkCoords)) {
      // this gets rid of the current chunk coord, but in an O(1) fashion
      ivec3_vec_swapAndPop(pWorldState->ready, (uint32_t)i);
      // add this to the unload coordinates
      ivec3_vec_push(pWorldState->tounload, chunkCoords);
    }
  }

  // finally process stuff on the unload list
  for (uint32_t i = 0;
       i < MAX_CHUNKS_TO_UNLOAD && ivec3_vec_len(pWorldState->tounload) > 0;
       i++) {
    ivec3_Chunk_KVPair c;
    ivec3_vec_pop(pWorldState->tounload, c.chunkCoord);

    // remove from hashmap
    ivec3_Chunk_KVPair *pChunk =
        hashmap_delete(pWorldState->chunk_map, c.chunkCoord);

    // put chunk geometry on garbage heap
    wld_pushGarbage(pWorldState, pChunk->pGeometry);
  }
}

static bool wld_delete_Geometries(const void *item, void *udata) {
  const ivec3_Chunk_KVPair *pChunk = item;
  const VkDevice device = udata;
  if (pChunk->pGeometry != NULL) {
    delete_ChunkGeometry(pChunk->pGeometry, device);
  }
  return true;
}

void wld_delete_WorldState( //
    WorldState *pWorldState //
) {
  // free simplex noise
  open_simplex_noise_free(pWorldState->noise1);

  // free vectors
  delete_ivec3_vec(&pWorldState->togenerate);
  delete_ivec3_vec(&pWorldState->tomesh);
  delete_ivec3_vec(&pWorldState->ready);
  delete_ivec3_vec(&pWorldState->tounload);

  // iterate through hashmap and free the geometries
  hashmap_scan(pWorldState->chunk_map, wld_delete_Geometries,
               pWorldState->device);

  // free the map
  hashmap_free(pWorldState->chunk_map);
}

void wld_count_vertexBuffers(     //
    uint32_t *pVertexBufferCount, //
    const WorldState *pWorldState //
) {

  uint32_t count = 0;
  for (uint32_t i = 0; i < ivec3_vec_len(pWorldState->ready); i++) {
    // get coord
    ivec3_Chunk_KVPair lookup_tmp;
    ivec3_vec_get(pWorldState->ready, i, lookup_tmp.chunkCoord);

    // get chunk
    ivec3_Chunk_KVPair *pChunk =
        hashmap_get(pWorldState->chunk_map, lookup_tmp.chunkCoord);

    // if chunk not empty add
    if (pChunk->pGeometry->vertexCount > 0) {
      count++;
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

  uint32_t count = 0;
  for (uint32_t i = 0; i < ivec3_vec_len(pWorldState->ready); i++) {
    // get coord
    ivec3_Chunk_KVPair lookup_tmp;
    ivec3_vec_get(pWorldState->ready, i, lookup_tmp.chunkCoord);

    // get chunk
    ivec3_Chunk_KVPair *pChunk =
        hashmap_get(pWorldState->chunk_map, lookup_tmp.chunkCoord);

    // write data
    if (pChunk->pGeometry->vertexCount > 0) {
      pVertexCounts[count] = pChunk->pGeometry->vertexCount;
      pVertexBuffers[count] = pChunk->pGeometry->vertexBuffer;
      count++;
    }
  }
}

void wld_set_center(         //
    WorldState *pWorldState, //
    const ivec3 centerLoc    //
) {
  // set our center location
  ivec3_dup(pWorldState->centerLoc, centerLoc);

  // initialize all of our neighboring chunks to be on the load list
  for (uint32_t x = 0; x < RENDER_DISTANCE_X; x++) {
    for (uint32_t y = 0; y < RENDER_DISTANCE_Y; y++) {
      for (uint32_t z = 0; z < RENDER_DISTANCE_Z; z++) {
        // push the chunk into our vector
        ivec3 tmp;
        arrayCoords_to_worldChunkCoords(tmp, centerLoc, x, y, z);
        ivec3_vec_push(pWorldState->togenerate, tmp);
      }
    }
  }
}
