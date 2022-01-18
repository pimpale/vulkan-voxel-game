#include <assert.h>
#include <math.h>
#include <stdlib.h>

#include "block.h"
#include "world.h"

#define WORKER_THREADS 16

// max chunks to mesh per tick
#define MAX_CHUNKS_TO_MESH 1
// max chunks to unload per tick
#define MAX_CHUNKS_TO_UNLOAD 10

// how many chunks to render
#define RENDER_RADIUS_X 3
#define RENDER_RADIUS_Y 3
#define RENDER_RADIUS_Z 3

struct ChunkGeometry_s {
  uint32_t vertexCount;
  // these 2 are only defined if vertexCount > 0
  VkBuffer vertexBuffer;
  VkDeviceMemory vertexBufferMemory;
};

static ErrVal
new_VertexBuffer(VkBuffer *pBuffer, VkDeviceMemory *pBufferMemory,
                 const Vertex *pVertices, const uint32_t vertexCount,
                 const VkDevice device, const VkPhysicalDevice physicalDevice,
                 const VkCommandPool commandPool, const VkQueue queue) {

  /* Construct staging buffers */
  VkDeviceSize bufferSize = sizeof(Vertex) * vertexCount;
  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  ErrVal stagingBufferCreateResult = new_Buffer_DeviceMemory(
      &stagingBuffer, &stagingBufferMemory, bufferSize, physicalDevice, device,
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  if (stagingBufferCreateResult != ERR_OK) {
    LOG_ERROR(
        ERR_LEVEL_FATAL,
        "failed to create vertex buffer: failed to create staging buffer");
    PANIC();
  }

  // Copy data to staging buffer, making sure to clean up leaks
  copyToDeviceMemory(&stagingBufferMemory, bufferSize, pVertices, device);

  /* Create vertex buffer and allocate memory for it */
  ErrVal vertexBufferCreateResult = new_Buffer_DeviceMemory(
      pBuffer, pBufferMemory, bufferSize, physicalDevice, device,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  /* Handle errors */
  if (vertexBufferCreateResult != ERR_OK) {
    /* Delete the temporary staging buffers */
    LOG_ERROR(ERR_LEVEL_FATAL, "failed to create vertex buffer");
    PANIC();
  }

  /* Copy the data over from the staging buffer to the vertex buffer */
  copyBuffer(*pBuffer, stagingBuffer, bufferSize, commandPool, queue, device);

  /* Delete the temporary staging buffers */
  delete_Buffer(&stagingBuffer, device);
  delete_DeviceMemory(&stagingBufferMemory, device);

  return (ERR_OK);
}

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
  c->vertexCount = wu_countChunkDataVertexes(data);
  if (c->vertexCount > 0) {
    // write mesh to vertex
    Vertex *vertexData = malloc(c->vertexCount * sizeof(Vertex));
    wu_getVertexesChunkData(vertexData, chunkOffset, data);
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
  ChunkData data;
  volatile bool initialized;
} ChunkDataState;

typedef struct {
  ivec3 chunkCoord;
  ChunkDataState *pDataAndState;
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
    worldgen_state *wgstate,              //
    const VkQueue queue,                  //
    const VkCommandPool commandPool,      //
    const VkDevice device,                //
    const VkPhysicalDevice physicalDevice //
) {
  pWorldState->wgstate = wgstate;
  // set center location
  ivec3_dup(pWorldState->centerLoc, centerLoc);

  // copy vulkan
  pWorldState->device = device;
  pWorldState->physicalDevice = physicalDevice;
  pWorldState->queue = queue;
  pWorldState->commandPool = commandPool;

  // initialize stacks to empty
  new_ivec3_vec(&pWorldState->togenerate);
  new_ivec3_vec(&pWorldState->generating);
  new_ivec3_vec(&pWorldState->tomesh);
  new_ivec3_vec(&pWorldState->ready);
  new_ivec3_vec(&pWorldState->tounload);

  // initialize threadpool
  pWorldState->pool = threadpool_create(WORKER_THREADS, MAX_QUEUE, 0);

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

  // set up highlight
  pWorldState->has_highlight = false;

  ErrVal highlightBufferCreateResult = new_Buffer_DeviceMemory(
      &pWorldState->highlightVertexBuffer,
      &pWorldState->highlightVertexBufferMemory, sizeof(Vertex) * 6,
      physicalDevice, device,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  if (highlightBufferCreateResult != ERR_OK) {
    LOG_ERROR(ERR_LEVEL_FATAL, "failed to create highlight vertex buffer");
    PANIC();
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

// argument struct that the worker thread takes ownership of
typedef struct {
  ivec3 worldChunkCoord;
  ChunkDataState *pDataAndState;
  const worldgen_state *pWgstate;
} WorkerThreadData;

static void worker_generate_chunk(UNUSED uint32_t id, void *arg) {
  WorkerThreadData *pwtd = arg;

  assert(!pwtd->pDataAndState->initialized);

  // generate chunk and then set intitialized to true
  worldgen_state_gen_chunk(&pwtd->pDataAndState->data, pwtd->worldChunkCoord,
                           pwtd->pWgstate);
  pwtd->pDataAndState->initialized = true;

  // free argument
  free(pwtd);
}

void wld_update(            //
    WorldState *pWorldState //
) {
  // process stuff off the togenerate list
  for (uint32_t i = 0; ivec3_vec_len(pWorldState->togenerate) > 0; i++) {
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

    // right now, we don't have any geometry or data
    // We set the initialized flag to false to signal that we haven't yet
    // generated it
    c.pDataAndState = malloc(sizeof(ChunkDataState));
    c.pDataAndState->initialized = false;
    c.pGeometry = NULL;

    // hashmap will clone the chunk to load
    hashmap_set(pWorldState->chunk_map, &c);

    WorkerThreadData *arg = malloc(sizeof(WorkerThreadData));
    *arg = (WorkerThreadData){.pDataAndState = c.pDataAndState,
                              .pWgstate = pWorldState->wgstate,
                              .worldChunkCoord = V3(c.chunkCoord)};

    threadpool_error_t e =
        threadpool_add(pWorldState->pool, worker_generate_chunk, arg, 0);
    if (e != 0) {
      LOG_ERROR(ERR_LEVEL_FATAL, "couldn't add task to threadpool!");
      PANIC();
    }
    // push the chunk coord to the chunks to generating vector
    ivec3_vec_push(pWorldState->generating, c.chunkCoord);
  }

  // process stuff on the generating list
  for (int32_t i = (int32_t)ivec3_vec_len(pWorldState->generating) - 1; i >= 0;
       i--) {
    ivec3_Chunk_KVPair key;
    ivec3_vec_get(pWorldState->generating, (uint32_t)i, key.chunkCoord);

    ivec3_Chunk_KVPair *generating = hashmap_get(pWorldState->chunk_map, &key);

    // check if the data is initialized
    if (generating->pDataAndState->initialized) {
      // this gets rid of the current chunk coord, but in an O(1) fashion
      ivec3_vec_swapAndPop(pWorldState->generating, (uint32_t)i);
      // add this to the tomesh coordinates
      ivec3_vec_push(pWorldState->tomesh, key.chunkCoord);
    }
  }

  // process stuff on the to mesh list
  for (uint32_t i = 0;
       i < MAX_CHUNKS_TO_MESH && ivec3_vec_len(pWorldState->tomesh) > 0; i++) {
    ivec3_Chunk_KVPair chunkToMesh;
    ivec3_vec_pop(pWorldState->tomesh, chunkToMesh.chunkCoord);

    ivec3_Chunk_KVPair *pChunk =
        hashmap_get(pWorldState->chunk_map, &chunkToMesh);

    if (pChunk->pGeometry != NULL) {
      // if some data already exists, place this geometry in the Garbage heap,
      // and make a new one
      wld_pushGarbage(pWorldState, pChunk->pGeometry);
    }
    pChunk->pGeometry = malloc(sizeof(ChunkGeometry));

    vec3 chunkOffset;
    worldChunkCoords_to_blockCoords(chunkOffset, pChunk->chunkCoord);

    new_ChunkGeometry(pChunk->pGeometry, &pChunk->pDataAndState->data,
                      chunkOffset, pWorldState->device,
                      pWorldState->physicalDevice, pWorldState->commandPool,
                      pWorldState->queue);

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

    free(pChunk->pDataAndState);

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
  free(pChunk->pDataAndState);
  return true;
}

void wld_delete_WorldState( //
    WorldState *pWorldState //
) {
  // wait for generating blocks to finish
  threadpool_destroy(pWorldState->pool, threadpool_graceful);

  // delete any blocks in the to generate stack
  ivec3_vec_clear(pWorldState->togenerate);

  // go through each 

  // clear the garbage
  wld_clearGarbage(pWorldState);
  // free garbage heap
  free(pWorldState->garbage_data);

  // free vectors
  delete_ivec3_vec(&pWorldState->togenerate);
  delete_ivec3_vec(&pWorldState->generating);
  delete_ivec3_vec(&pWorldState->tomesh);
  delete_ivec3_vec(&pWorldState->ready);
  delete_ivec3_vec(&pWorldState->tounload);

  // iterate through hashmap and free the geometries and data
  hashmap_scan(pWorldState->chunk_map, wld_delete_HashmapData,
               pWorldState->device);

  // free the map
  hashmap_free(pWorldState->chunk_map);

  // free the highlights
  delete_Buffer(&pWorldState->highlightVertexBuffer, pWorldState->device);
  delete_DeviceMemory(&pWorldState->highlightVertexBufferMemory,
                      pWorldState->device);
}

void wld_count_vertexBuffers(     //
    uint32_t *pVertexBufferCount, //
    const WorldState *pWorldState //
) {

  uint32_t count = 0;

  if (pWorldState->has_highlight) {
    count++;
  }



  for (uint32_t i = 0; i < ivec3_vec_len(pWorldState->ready); i++) {
    // get coord
    ivec3_Chunk_KVPair lookup_tmp;
    ivec3_vec_get(pWorldState->ready, i, lookup_tmp.chunkCoord);

    // get chunk
    ivec3_Chunk_KVPair *pChunk =
        hashmap_get(pWorldState->chunk_map, &lookup_tmp);

    // if chunk not empty add
    if (pChunk->pGeometry->vertexCount > 0) {
      count++;
    }
  }

  for (uint32_t i = 0; i < ivec3_vec_len(pWorldState->tomesh); i++) {
    // get coord
    ivec3_Chunk_KVPair lookup_tmp;
    ivec3_vec_get(pWorldState->tomesh, i, lookup_tmp.chunkCoord);

    // get chunk
    ivec3_Chunk_KVPair *pChunk =
        hashmap_get(pWorldState->chunk_map, &lookup_tmp);

    // if chunk exists and is not empty add
    if (pChunk->pGeometry != NULL && pChunk->pGeometry->vertexCount > 0) {
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

  if (pWorldState->has_highlight) {
    pVertexCounts[count] = 6;
    pVertexBuffers[count] = pWorldState->highlightVertexBuffer;
    count++;
  }

  for (uint32_t i = 0; i < ivec3_vec_len(pWorldState->ready); i++) {
    // get coord
    ivec3_Chunk_KVPair lookup_tmp;
    ivec3_vec_get(pWorldState->ready, i, lookup_tmp.chunkCoord);

    // get chunk
    ivec3_Chunk_KVPair *pChunk =
        hashmap_get(pWorldState->chunk_map, &lookup_tmp);

    // write data
    if (pChunk->pGeometry->vertexCount > 0) {
      pVertexCounts[count] = pChunk->pGeometry->vertexCount;
      pVertexBuffers[count] = pChunk->pGeometry->vertexBuffer;
      count++;
    }
  }

  for (uint32_t i = 0; i < ivec3_vec_len(pWorldState->tomesh); i++) {
    // get coord
    ivec3_Chunk_KVPair lookup_tmp;
    ivec3_vec_get(pWorldState->tomesh, i, lookup_tmp.chunkCoord);

    // get chunk
    ivec3_Chunk_KVPair *pChunk =
        hashmap_get(pWorldState->chunk_map, &lookup_tmp);

    // write data
    if (pChunk->pGeometry != NULL && pChunk->pGeometry->vertexCount > 0) {
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

bool wld_get_block_at( //
    BlockIndex *pBlock,       //
    WorldState *pWorldState,  //
    const ivec3 iBlockCoords  //
) {
  vec3 blockCoords;
  ivec3_to_vec3(blockCoords, iBlockCoords);

  // get world chunk coord
  ivec3_Chunk_KVPair lookup_tmp;
  blockCoords_to_worldChunkCoords(lookup_tmp.chunkCoord, blockCoords);

  // get chunk
  ivec3_Chunk_KVPair *pChunk = hashmap_get(pWorldState->chunk_map, &lookup_tmp);

  if (pChunk == NULL || !pChunk->pDataAndState->initialized) {
    return false;
  }

  // get corner block of world chunk
  ivec3 iBlockCoord_Corner;
  worldChunkCoords_to_iBlockCoords(iBlockCoord_Corner, pChunk->chunkCoord);

  // get intra-chunk offset
  ivec3 intraChunkOffset;
  ivec3_sub(intraChunkOffset, iBlockCoords, iBlockCoord_Corner);

  assert(intraChunkOffset[0] + iBlockCoord_Corner[0] == iBlockCoords[0]);

  // chunk index
  ivec3 chunkIndex = {
      (intraChunkOffset[0] % CHUNK_X_SIZE + CHUNK_X_SIZE) % CHUNK_X_SIZE,
      (intraChunkOffset[1] % CHUNK_Y_SIZE + CHUNK_Y_SIZE) % CHUNK_Y_SIZE,
      (intraChunkOffset[2] % CHUNK_Z_SIZE + CHUNK_Z_SIZE) % CHUNK_Z_SIZE};

  *pBlock = pChunk->pDataAndState->data
                .blocks[chunkIndex[0]][chunkIndex[1]][chunkIndex[2]];
  return true;
}

bool wld_set_block_at( //
    BlockIndex block,         //
    WorldState *pWorldState,  //
    const ivec3 iBlockCoords  //
) {
  vec3 blockCoords;
  ivec3_to_vec3(blockCoords, iBlockCoords);

  // get world chunk coord
  ivec3_Chunk_KVPair lookup_tmp;
  blockCoords_to_worldChunkCoords(lookup_tmp.chunkCoord, blockCoords);

  // get chunk
  ivec3_Chunk_KVPair *pChunk = hashmap_get(pWorldState->chunk_map, &lookup_tmp);

  if (pChunk == NULL || !pChunk->pDataAndState->initialized) {
    return false;
  }

  // get corner block of world chunk
  ivec3 iBlockCoord_Corner;
  worldChunkCoords_to_iBlockCoords(iBlockCoord_Corner, pChunk->chunkCoord);

  // get intra-chunk offset
  ivec3 intraChunkOffset;
  ivec3_sub(intraChunkOffset, iBlockCoords, iBlockCoord_Corner);

  assert(intraChunkOffset[0] + iBlockCoord_Corner[0] == iBlockCoords[0]);

  // chunk index
  ivec3 chunkIndex = {
      (intraChunkOffset[0] % CHUNK_X_SIZE + CHUNK_X_SIZE) % CHUNK_X_SIZE,
      (intraChunkOffset[1] % CHUNK_Y_SIZE + CHUNK_Y_SIZE) % CHUNK_Y_SIZE,
      (intraChunkOffset[2] % CHUNK_Z_SIZE + CHUNK_Z_SIZE) % CHUNK_Z_SIZE};

  pChunk->pDataAndState->data
      .blocks[chunkIndex[0]][chunkIndex[1]][chunkIndex[2]] = block;

  // if the block is in the ready vec, then put it back into the needs_mesh
  for (int32_t i = (int32_t)ivec3_vec_len(pWorldState->ready) - 1; i >= 0;
       i--) {
    ivec3 chunkCoords;
    ivec3_vec_get(pWorldState->ready, (uint32_t)i, chunkCoords);
    if (ivec3_eq(pChunk->chunkCoord, chunkCoords)) {
      // this gets rid of the current chunk coord, but in an O(1) fashion
      ivec3_vec_swapAndPop(pWorldState->ready, (uint32_t)i);
      // add this to the unload coordinates
      ivec3_vec_push(pWorldState->tomesh, chunkCoords);
    }
  }

  return true;
}

static int32_t signum(float x) { return x > 0 ? 1 : x < 0 ? -1 : 0; }

static float intbound(float s, float ds) {
  // Some kind of edge case, see:
  // http://gamedev.stackexchange.com/questions/47362/cast-ray-to-select-block-in-voxel-game#comment160436_49423
  bool sIsInteger = roundf(s) == s;
  if (ds < 0 && sIsInteger)
    return 0;

  float ceils;
  if (s == 0.0f) {
    ceils = 1.0f;
  } else {
    ceils = ceilf(s);
  }

  return (ds > 0 ? ceils - s : s - floorf(s)) / fabsf(ds);
}

const static float epsilonf = 0.001f;

/**
 * Call the callback with (x,y,z,value,face) of all blocks along the line
 * segment from point 'origin' in vector direction 'direction' of length
 * 'radius'. 'radius' may be infinite.
 *
 * 'face' is the normal vector of the face of that block that was entered.
 * It should not be used after the callback returns.
 *
 * If the callback returns a true value, the traversal will be stopped.
 */
bool wld_trace_to_solid(      //
    ivec3 dest_iBlockCoords,  //
    BlockFaceKind *dest_face, //
    const vec3 origin,        //
    const vec3 direction,     //
    const uint32_t max_dist,  //
    WorldState *pWorldState   //
) {
  // From "A Fast Voxel Traversal Algorithm for Ray Tracing"
  // by John Amanatides and Andrew Woo, 1987
  // <http://www.cse.yorku.ca/~amana/research/grid.pdf>
  // <http://citeseer.ist.psu.edu/viewdoc/summary?doi=10.1.1.42.3443>
  // Extensions to the described algorithm:
  //   • Imposed a distance limit.
  //   • The face passed through to reach the current cube is provided to
  //     the callback.

  // The foundation of this algorithm is a parameterized representation of
  // the provided ray,
  //                    origin + t * direction,
  // except that t is not actually stored; rather, at any given point in the
  // traversal, we keep track of the *greater* t values which we would have
  // if we took a step sufficient to cross a cube boundary along that axis
  // (i.e. change the integer part of the coordinate) in the variables
  // tMaxX, tMaxY, and tMaxZ.

  // Cube containing origin point.
  int32_t x = (int32_t)(floorf(origin[0]));
  int32_t y = (int32_t)(floorf(origin[1]));
  int32_t z = (int32_t)(floorf(origin[2]));
  // Break out direction vector.
  float dx = direction[0];
  float dy = direction[1];
  float dz = direction[2];
  // Direction to increment x,y,z when stepping.
  int32_t stepX = signum(dx);
  int32_t stepY = signum(dy);
  int32_t stepZ = signum(dz);
  // See description above. The initial values depend on the fractional
  // part of the origin.
  float tMaxX = intbound(origin[0], dx);
  float tMaxY = intbound(origin[1], dy);
  float tMaxZ = intbound(origin[2], dz);
  // The change in t when taking a step (always positive).
  float tDeltaX = (float)stepX / dx;
  float tDeltaY = (float)stepY / dy;
  float tDeltaZ = (float)stepZ / dz;

  // Avoids an infinite loop.
  // reject if the direction is zero
  assert(
      !(fabsf(dx) < epsilonf && fabsf(dy) < epsilonf && fabsf(dz) < epsilonf));

  // Rescale from units of 1 cube-edge to units of 'direction' so we can
  // compare with 't'.
  float radius = (float)(max_dist) / sqrtf(dx * dx + dy * dy + dz * dz);
  while (true) {
    // get block here
    ivec3 coord = {x, y, z};
    BlockIndex bi;
    bool success = wld_get_block_at(&bi, pWorldState, coord);
    if (!success) {
      break;
    }

    if (!BLOCKS[bi].transparent) {
      ivec3_dup(dest_iBlockCoords, coord);
      return true;
    }

    // tMaxX stores the t-value at which we cross a cube boundary along the
    // X axis, and similarly for Y and Z. Therefore, choosing the least tMax
    // chooses the closest cube boundary. Only the first case of the four
    // has been commented in detail.
    if (tMaxX < tMaxY) {
      if (tMaxX < tMaxZ) {
        if (tMaxX > radius)
          break;
        // Update which cube we are now in.
        x += stepX;
        // Adjust tMaxX to the next X-oriented boundary crossing.
        tMaxX += tDeltaX;
        // Record the normal vector of the cube face we entered.
        *dest_face = stepX == 1 ? Block_LEFT : Block_RIGHT;
      } else {
        if (tMaxZ > radius)
          break;
        z += stepZ;
        tMaxZ += tDeltaZ;
        *dest_face = stepZ == 1 ? Block_BACK : Block_FRONT;
      }
    } else {
      if (tMaxY < tMaxZ) {
        if (tMaxY > radius)
          break;
        y += stepY;
        tMaxY += tDeltaY;
        *dest_face = stepY == 1 ? Block_UP : Block_DOWN;
      } else {
        // Identical to the second case, repeated for simplicity in
        // the conditionals.
        if (tMaxZ > radius)
          break;
        z += stepZ;
        tMaxZ += tDeltaZ;
        *dest_face = stepZ == 1 ? Block_BACK : Block_FRONT;
      }
    }
  }

  return false;
}

/// highlight updates the world's highlighted block and
void wld_highlight_face(      //
    const ivec3 iBlockCoords, //
    BlockFaceKind face,       //
    WorldState *pWorldState   //
) {
  // get new highlight buffer
  Vertex highlightVertexes[6];
  wu_getVertexesHighlight(highlightVertexes, iBlockCoords, face);
  // update buffer
  updateBuffer(pWorldState->highlightVertexBuffer, highlightVertexes,
               sizeof(highlightVertexes), pWorldState->commandPool,
               pWorldState->queue, pWorldState->device);
  pWorldState->has_highlight = true;
}

/// highlight updates the world's highlighted block and
void wld_clear_highlight_face( //
    WorldState *pWorldState    //
) {
  pWorldState->has_highlight = false;
}
