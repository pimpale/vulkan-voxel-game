#include <assert.h>
#include <math.h>
#include <stdlib.h>

#include "block.h"
#include "world.h"

// max chunks to generate per tick
#define MAX_CHUNKS_TO_GENERATE 1
// max chunks to mesh per tick
#define MAX_CHUNKS_TO_MESH 10
// max chunks to unload per tick
#define MAX_CHUNKS_TO_UNLOAD 10

// how many chunks to render
#define RENDER_RADIUS_X 3
#define RENDER_RADIUS_Y 3
#define RENDER_RADIUS_Z 3

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

#define V3(x)                                                                  \
  { x[0], x[1], x[2] }

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
        BlockIndex bi = pCd->blocks[x][y][z];
        // check that its not transparent
        if (BLOCKS[bi].transparent) {
          continue;
        }

        // get chunk location
        const float fx = (float)x + offset[0];
        const float fy = (float)y + offset[1];
        const float fz = (float)z + offset[2];

        // calculate vertexes
        const vec3 v000 = {fx + 0, fy + 0, fz + 0};
        const vec3 v100 = {fx + 1, fy + 0, fz + 0};
        const vec3 v001 = {fx + 0, fy + 0, fz + 1};
        const vec3 v101 = {fx + 1, fy + 0, fz + 1};
        const vec3 v010 = {fx + 0, fy + 1, fz + 0};
        const vec3 v110 = {fx + 1, fy + 1, fz + 0};
        const vec3 v011 = {fx + 0, fy + 1, fz + 1};
        const vec3 v111 = {fx + 1, fy + 1, fz + 1};

        const float xoff = BLOCK_TILE_TEX_XSIZE;
        const float yoff = BLOCK_TILE_TEX_YSIZE;

        // clang-format off

        // left face
        if (x == 0 || BLOCKS[pCd->blocks[x - 1][y][z]].transparent) {
          const float bx = BLOCK_TILE_TEX_XSIZE*Block_LEFT;
          const float by = BLOCK_TILE_TEX_YSIZE*bi;
          pVertexes[i++] = (Vertex){.position=V3(v000), .texCoords= {bx+0.0f, by+0.0f}};
          pVertexes[i++] = (Vertex){.position=V3(v010), .texCoords= {bx+0.0f, by+yoff}};
          pVertexes[i++] = (Vertex){.position=V3(v001), .texCoords= {bx+xoff, by+0.0f}};
          pVertexes[i++] = (Vertex){.position=V3(v001), .texCoords= {bx+xoff, by+0.0f}};
          pVertexes[i++] = (Vertex){.position=V3(v010), .texCoords= {bx+0.0f, by+yoff}};
          pVertexes[i++] = (Vertex){.position=V3(v011), .texCoords= {bx+xoff, by+yoff}};
        }
        // right face
        if (x == CHUNK_X_SIZE - 1 ||
            BLOCKS[pCd->blocks[x + 1][y][z]].transparent) {
          const float bx = BLOCK_TILE_TEX_XSIZE*Block_RIGHT;
          const float by = BLOCK_TILE_TEX_YSIZE*bi;
          pVertexes[i++] = (Vertex){.position=V3(v100), .texCoords= {bx+xoff, by+0.0f}};
          pVertexes[i++] = (Vertex){.position=V3(v101), .texCoords= {bx+0.0f, by+0.0f}};
          pVertexes[i++] = (Vertex){.position=V3(v110), .texCoords= {bx+xoff, by+yoff}};
          pVertexes[i++] = (Vertex){.position=V3(v101), .texCoords= {bx+0.0f, by+0.0f}};
          pVertexes[i++] = (Vertex){.position=V3(v111), .texCoords= {bx+0.0f, by+yoff}};
          pVertexes[i++] = (Vertex){.position=V3(v110), .texCoords= {bx+xoff, by+yoff}};
        }

        // upper face
        if (y == 0 || BLOCKS[pCd->blocks[x][y - 1][z]].transparent) {
          const float bx = BLOCK_TILE_TEX_XSIZE*Block_UP;
          const float by = BLOCK_TILE_TEX_YSIZE*bi;
          pVertexes[i++] = (Vertex){.position=V3(v001), .texCoords={bx+0.0f, by+yoff}};
          pVertexes[i++] = (Vertex){.position=V3(v100), .texCoords={bx+xoff, by+0.0f}};
          pVertexes[i++] = (Vertex){.position=V3(v000), .texCoords={bx+0.0f, by+0.0f}};
          pVertexes[i++] = (Vertex){.position=V3(v001), .texCoords={bx+0.0f, by+yoff}};
          pVertexes[i++] = (Vertex){.position=V3(v101), .texCoords={bx+xoff, by+yoff}};
          pVertexes[i++] = (Vertex){.position=V3(v100), .texCoords={bx+xoff, by+0.0f}};
        }
        // lower face
        if (y == CHUNK_Y_SIZE - 1 ||
            BLOCKS[pCd->blocks[x][y + 1][z]].transparent) {
          const float bx = BLOCK_TILE_TEX_XSIZE*Block_DOWN;
          const float by = BLOCK_TILE_TEX_YSIZE*bi;
          pVertexes[i++] = (Vertex){.position=V3(v010), .texCoords={bx+xoff, by+0.0f}};
          pVertexes[i++] = (Vertex){.position=V3(v110), .texCoords={bx+0.0f, by+0.0f}};
          pVertexes[i++] = (Vertex){.position=V3(v011), .texCoords={bx+xoff, by+yoff}};
          pVertexes[i++] = (Vertex){.position=V3(v110), .texCoords={bx+0.0f, by+0.0f}};
          pVertexes[i++] = (Vertex){.position=V3(v111), .texCoords={bx+0.0f, by+yoff}};
          pVertexes[i++] = (Vertex){.position=V3(v011), .texCoords={bx+xoff, by+yoff}};
        }

        // back face
        if (z == 0 || BLOCKS[pCd->blocks[x][y][z - 1]].transparent) {
          const float bx = BLOCK_TILE_TEX_XSIZE*Block_BACK;
          const float by = BLOCK_TILE_TEX_YSIZE*bi;
          pVertexes[i++] = (Vertex){.position=V3(v000), .texCoords= {bx+xoff, by+0.0f}};
          pVertexes[i++] = (Vertex){.position=V3(v100), .texCoords= {bx+0.0f, by+0.0f}};
          pVertexes[i++] = (Vertex){.position=V3(v010), .texCoords= {bx+xoff, by+yoff}};
          pVertexes[i++] = (Vertex){.position=V3(v100), .texCoords= {bx+0.0f, by+0.0f}};
          pVertexes[i++] = (Vertex){.position=V3(v110), .texCoords= {bx+0.0f, by+yoff}};
          pVertexes[i++] = (Vertex){.position=V3(v010), .texCoords= {bx+xoff, by+yoff}};
        }

        // front face
        if (z == CHUNK_Z_SIZE - 1 ||
            BLOCKS[pCd->blocks[x][y][z + 1]].transparent) {
          const float bx = BLOCK_TILE_TEX_XSIZE*Block_FRONT;
          const float by = BLOCK_TILE_TEX_YSIZE*bi;
          pVertexes[i++] = (Vertex){.position=V3(v011), .texCoords= {bx+0.0f, by+yoff}};
          pVertexes[i++] = (Vertex){.position=V3(v101), .texCoords= {bx+xoff, by+0.0f}};
          pVertexes[i++] = (Vertex){.position=V3(v001), .texCoords= {bx+0.0f, by+0.0f}};
          pVertexes[i++] = (Vertex){.position=V3(v011), .texCoords= {bx+0.0f, by+yoff}};
          pVertexes[i++] = (Vertex){.position=V3(v111), .texCoords= {bx+xoff, by+yoff}};
          pVertexes[i++] = (Vertex){.position=V3(v101), .texCoords= {bx+xoff, by+0.0f}};
        }
        // clang-format on
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
  for (uint32_t x = 0; x < CHUNK_X_SIZE; x++) {
    for (uint32_t y = 0; y < CHUNK_Y_SIZE; y++) {
      for (uint32_t z = 0; z < CHUNK_Z_SIZE; z++) {
        // calculate world coordinates in blocks
        double wx = x + (double)chunkOffset[0];
        double wy = y + (double)chunkOffset[1];
        double wz = z + (double)chunkOffset[2];
        double val = open_simplex_noise3(noiseCtx, wx / scale1, wy / scale1,
                                         wz / scale1);
        double val2 = open_simplex_noise3(noiseCtx, wx / scale1, (wy - 1) / scale1,
                                         wz / scale1);
        if (val > 0 && val2 < 0) {
          pCd->blocks[x][y][z] = 1; // grass
        } else if(val > 0) {
          pCd->blocks[x][y][z] = 2; // grass
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
  if (geometry->vertexCount > 0) {
    delete_Buffer(&geometry->vertexBuffer, device);
    delete_DeviceMemory(&geometry->vertexBufferMemory, device);
  }
}

typedef struct {
  ivec3 chunkCoord;
  ChunkData *pData;
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

  // initialize garbage heap
  pWorldState->garbage_cap = 16;
  pWorldState->garbage_data =
      malloc(pWorldState->garbage_cap * sizeof(ChunkGeometry *));
  pWorldState->garbage_len = 0;

  // initialize hash maps
  pWorldState->chunk_map =
      hashmap_new(sizeof(ivec3_Chunk_KVPair), 0, 0, 0, ivec3_Chunk_KVPair_hash,
                  ivec3_Chunk_KVPair_compare, NULL, NULL);

  // initialize all of our neighboring chunks to be on the load list
  for (int32_t x = -RENDER_RADIUS_X; x <= RENDER_RADIUS_X; x++) {
    for (int32_t y = -RENDER_RADIUS_Y; y <= RENDER_RADIUS_Y; y++) {
      for (int32_t z = -RENDER_RADIUS_Z; z <= RENDER_RADIUS_Z; z++) {
        ivec3 chunkCoord;
        ivec3_add(chunkCoord, centerLoc, (ivec3){x, y, z});
        ivec3_vec_push(pWorldState->togenerate, chunkCoord);
      }
    }
  }
}

static bool wld_shouldBeLoaded(    //
    const WorldState *pWorldState, //
    const ivec3 worldChunkCoords   //
) {
  ivec3 disp;
  ivec3_sub(disp, worldChunkCoords, pWorldState->centerLoc);

  return (disp[0] >= -RENDER_RADIUS_X && disp[0] <= RENDER_RADIUS_X) &&
         (disp[1] >= -RENDER_RADIUS_Y && disp[1] <= RENDER_RADIUS_Y) &&
         (disp[2] >= -RENDER_RADIUS_Z && disp[2] <= RENDER_RADIUS_Z);
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
    ivec3_Chunk_KVPair c;
    ivec3_vec_pop(pWorldState->togenerate, c.chunkCoord);

    // check that we still even need to load this
    if (!wld_shouldBeLoaded(pWorldState, c.chunkCoord)) {
      continue;
    }

    // check we haven't already loaded this
    if (hashmap_get(pWorldState->chunk_map, &c) != NULL) {
      continue;
    }

    // generate chunk, we need to give it the block coordinate to generate at
    vec3 chunkOffset;
    worldChunkCoords_to_blockCoords(chunkOffset, c.chunkCoord);

    c.pData = malloc(sizeof(ChunkData));
    blocks_gen(c.pData, chunkOffset, pWorldState->noise1);

    // no geometry yet
    c.pGeometry = NULL;

    // push the chunk coord to the chunks to mesh
    ivec3_vec_push(pWorldState->tomesh, c.chunkCoord);

    // hashmap will clone the chunk to load
    hashmap_set(pWorldState->chunk_map, &c);
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
    }
    pChunk->pGeometry = malloc(sizeof(ChunkGeometry));

    vec3 chunkOffset;
    worldChunkCoords_to_blockCoords(chunkOffset, pChunk->chunkCoord);

    new_ChunkGeometry(pChunk->pGeometry, pChunk->pData, chunkOffset,
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

    free(pChunk->pData);

    // put chunk geometry on garbage pile
    wld_pushGarbage(pWorldState, pChunk->pGeometry);
  }
}

static bool wld_delete_HashmapData(const void *item, void *udata) {
  const ivec3_Chunk_KVPair *pChunk = item;
  const VkDevice device = udata;
  if (pChunk->pGeometry != NULL) {
    delete_ChunkGeometry(pChunk->pGeometry, device);
    free(pChunk->pGeometry);
  }
  free(pChunk->pData);
  return true;
}

void wld_delete_WorldState( //
    WorldState *pWorldState //
) {
  // clear the garbage
  wld_clearGarbage(pWorldState);
  // free garbage heap
  free(pWorldState->garbage_data);

  // free simplex noise
  open_simplex_noise_free(pWorldState->noise1);

  // free vectors
  delete_ivec3_vec(&pWorldState->togenerate);
  delete_ivec3_vec(&pWorldState->tomesh);
  delete_ivec3_vec(&pWorldState->ready);
  delete_ivec3_vec(&pWorldState->tounload);

  // iterate through hashmap and free the geometries and data
  hashmap_scan(pWorldState->chunk_map, wld_delete_HashmapData,
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
  for (int32_t x = -RENDER_RADIUS_X; x <= RENDER_RADIUS_X; x++) {
    for (int32_t y = -RENDER_RADIUS_Y; y <= RENDER_RADIUS_Y; y++) {
      for (int32_t z = -RENDER_RADIUS_Z; z <= RENDER_RADIUS_Z; z++) {
        ivec3 chunkCoord;
        ivec3_add(chunkCoord, centerLoc, (ivec3){x, y, z});
        ivec3_vec_push(pWorldState->togenerate, chunkCoord);
      }
    }
  }
}
