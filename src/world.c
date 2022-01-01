#include <assert.h>
#include <math.h>
#include <stdlib.h>

#include "block.h"
#include "world.h"

#define WORKER_THREADS 16

// max chunks to mesh per tick
#define MAX_CHUNKS_TO_MESH 3
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
    // We set the initialized flag to false to signal that we haven't yet generated it
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

    ivec3_Chunk_KVPair* generating = hashmap_get(pWorldState->chunk_map, &key);

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
        hashmap_get(pWorldState->chunk_map, chunkToMesh.chunkCoord);

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

  // update world stack, clearing the generating stack
  wld_update(pWorldState);

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


