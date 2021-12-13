#include <assert.h>
#include <math.h>
#include <pthread.h>
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

// subtasks are uninterruptible components
static void worker_subtask_mesh_chunk(     //
    Chunk *c,                              //
    const vec3 chunkOffset,                //
    const VkDevice device,                 //
    const VkPhysicalDevice physicalDevice, //
    const VkCommandPool commandPool,       //
    const VkQueue queue,                   //
    pthread_mutex_t *pQueueMutex           //
) {
  assert(c->state == wld_cs_NEEDS_MESH);

  // start working on c
  c->working = true;

  // set state to meshing
  c->state = wld_cs_MESHING;

  // count chunk vertexes
  c->geometry.vertexCount = blocks_count_vertexes_internal(&c->chunkData);

  if (c->geometry.vertexCount == 0) {
    c->state = wld_cs_EMPTY;
  } else {
    // write mesh to vertex
    Vertex *vertexData = malloc(c->geometry.vertexCount * sizeof(Vertex));
    blocks_mesh_internal(vertexData, chunkOffset, &c->chunkData);

    // create vertex buffer + backing memory
    pthread_mutex_lock(pQueueMutex);
    new_VertexBuffer(&c->geometry.vertexBuffer, &c->geometry.vertexBufferMemory,
                     vertexData, c->geometry.vertexCount, device,
                     physicalDevice, commandPool, queue);
    pthread_mutex_unlock(pQueueMutex);

    c->state = wld_cs_READY;
  }
  c->working = false;
}

typedef struct {
  Chunk *c;
  vec3 chunkOffset;
  struct osn_context *noiseCtx;
  VkDevice device;
  VkPhysicalDevice physicalDevice;
  VkCommandPool *commandPoolArr;
  VkQueue queue;
  pthread_mutex_t *pQueueMutex;
} wld_WorkerOwnedData;

// can be interrupted
static void worker_generate_chunk( //
    uint32_t thread_id,            //
    void *arg                      //
) {
  wld_WorkerOwnedData *wa = arg;

  assert(wa->c->state == wld_cs_NEEDS_GENERATE);

  // start working on c
  wa->c->working = true;
  // set state to generating
  wa->c->state = wld_cs_GENERATING;
  // work on generating
  blocks_gen(&wa->c->chunkData, wa->chunkOffset, wa->noiseCtx);
  wa->c->state = wld_cs_NEEDS_MESH;
  wa->c->working = false;

  if (!wa->c->chunk_dead) {
    // mesh
    worker_subtask_mesh_chunk(         //
        wa->c,                         //
        wa->chunkOffset,               //
        wa->device,                    //
        wa->physicalDevice,            //
        wa->commandPoolArr[thread_id], //
        wa->queue,                     //
        wa->pQueueMutex                //
    );
  }

  // now get rid of data
  free(wa);
}

void wld_new_WorldState(                  //
    WorldState *pWorldState,              //
    const ivec3 centerLoc,                //
    const uint32_t seed,                  //
    const uint32_t graphicsQueueFamily,   //
    const VkQueue queue,                  //
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

  // start mutex
  pWorldState->pQueueMutex = malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(pWorldState->pQueueMutex, NULL);

  // generate thread queues and pools
  for (uint32_t i = 0; i < RENDER_THREADS; i++) {
    if (new_CommandPool(&pWorldState->threadCommandPools[i], device,
                        graphicsQueueFamily) != ERR_OK) {
      LOG_ERROR(ERR_LEVEL_ERROR, "failed to create command pool");
      PANIC();
    }
  }

  // generate threads
  pWorldState->threadpool = threadpool_create(RENDER_THREADS, MAX_QUEUE, 0);

  // initialize vector lengths to zero
  pWorldState->deadchunk_len = 0;
  pWorldState->pinned_len = 0;

  // begin task of initializing

  // note that, x, y, and z are relative to the array
  for (uint32_t x = 0; x < RENDER_DISTANCE_X; x++) {
    for (uint32_t y = 0; y < RENDER_DISTANCE_Y; y++) {
      for (uint32_t z = 0; z < RENDER_DISTANCE_Z; z++) {
        // calculate chunk offset
        ivec3 worldChunkCoords;
        arrayCoords_to_worldChunkCoords(worldChunkCoords, centerLoc, x, y, z);

        // allocate data
        pWorldState->data[x][y][z] = malloc(sizeof(Chunk));
        pWorldState->data[x][y][z]->state = wld_cs_NEEDS_GENERATE;

        // construct data to be sent to task
        wld_WorkerOwnedData *wod = malloc(sizeof(wld_WorkerOwnedData));
        wod->c = pWorldState->data[x][y][z];
        worldChunkCoords_to_blockCoords(wod->chunkOffset, worldChunkCoords);
        wod->noiseCtx = pWorldState->noise1;
        wod->device = pWorldState->device;
        wod->physicalDevice = pWorldState->physicalDevice;
        wod->commandPoolArr = pWorldState->threadCommandPools;
        wod->queue = pWorldState->queue;
        wod->pQueueMutex = pWorldState->pQueueMutex;

        // the thread will take ownership of the worker owned data
        threadpool_error_t err = threadpool_add(pWorldState->threadpool,
                                                worker_generate_chunk, wod, 0);

        // check for error
        if (err != 0) {
          LOG_ERROR(ERR_LEVEL_FATAL, "threadpool failed to accept task");
          PANIC();
        }
      }
    }
  }
}

static void wld_delete_Chunk(Chunk *c, const VkDevice device) {
  // get rid of geometry
  if (c->state == wld_cs_PINNED || c->state == wld_cs_READY) {
    delete_Buffer(&c->geometry.vertexBuffer, device);
    delete_DeviceMemory(&c->geometry.vertexBufferMemory, device);
  }
}

void wld_delete_WorldState( //
    WorldState *pWorldState //
) {

  // first we wait for all of the tasks to finish
  threadpool_error_t err =
      threadpool_destroy(pWorldState->threadpool, threadpool_graceful);
  if (err != 0) {
    LOG_ERROR(ERR_LEVEL_FATAL, "threadpool failed to shut down");
    PANIC();
  }

  // free vulkan stuff that we own
  for (uint32_t i = 0; i < RENDER_THREADS; i++) {
    delete_CommandPool(&pWorldState->threadCommandPools[i],
                       pWorldState->device);
    // queues are automatically deleted, we don't have to do anything
  }

  // end mutex
  pthread_mutex_destroy(pWorldState->pQueueMutex);
  free(pWorldState->pQueueMutex);

  // free simplex noise
  open_simplex_noise_free(pWorldState->noise1);

  // go through chunks and free and delete
  for (uint32_t x = 0; x < RENDER_DISTANCE_X; x++) {
    for (uint32_t y = 0; y < RENDER_DISTANCE_Y; y++) {
      for (uint32_t z = 0; z < RENDER_DISTANCE_Z; z++) {
        Chunk *c = pWorldState->data[x][y][z];
        wld_delete_Chunk(c, pWorldState->device);
        free(c);
      }
    }
  }

  // don't have to do anything to pinned chunk vector
  // (the contents are already destroyed)

  // free everything in the dead chunk vector
  // (these weren't in the grid)
  for (uint32_t i = 0; i < pWorldState->deadchunk_len; i++) {
    Chunk *c = pWorldState->deadchunk_vec[i];
    wld_delete_Chunk(c, pWorldState->device);
    free(c);
  }
}

void wld_doGeometryUpdates_async( //
    WorldState *pWorldState       //
) {
  // clear the pinned chunks vec, then go through the array and update with all
  // that are in states RENDERING or READY

  pWorldState->pinned_len = 0;

  for (uint32_t x = 0; x < RENDER_DISTANCE_X; x++) {
    for (uint32_t y = 0; y < RENDER_DISTANCE_Y; y++) {
      for (uint32_t z = 0; z < RENDER_DISTANCE_Z; z++) {
        switch (pWorldState->data[x][y][z]->state) {
        case wld_cs_PINNED:
          pWorldState->pinned_vec[pWorldState->pinned_len] =
              pWorldState->data[x][y][z];
          break;
        case wld_cs_READY:
          pWorldState->pinned_vec[pWorldState->pinned_len] =
              pWorldState->data[x][y][z];
          pWorldState->pinned_len++;
          // make pinned
          pWorldState->data[x][y][z]->state = wld_cs_PINNED;
          break;
        default:
          break;
        }
      }
    }
  }

  // delete everything in the dead chunk vector, since we no longer need it
  for (uint32_t i = 0; i < pWorldState->deadchunk_len; i++) {
    Chunk *c = pWorldState->deadchunk_vec[i];
    wld_delete_Chunk(c, pWorldState->device);
    free(c);
  }
}

void wld_count_vertexBuffers(     //
    uint32_t *pVertexBufferCount, //
    const WorldState *pWorldState //
) {
  *pVertexBufferCount = pWorldState->pinned_len;
}

// these buffers are for reading only! don't delete or modify
void wld_getVertexBuffers(        //
    VkBuffer *pVertexBuffers,     //
    uint32_t *pVertexCounts,      //
    const WorldState *pWorldState //
) {
  for (uint32_t i = 0; i < pWorldState->pinned_len; i++) {
    pVertexCounts[i] = pWorldState->pinned_vec[i]->geometry.vertexCount;
    pVertexBuffers[i] = pWorldState->pinned_vec[i]->geometry.vertexBuffer;
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

// this data is owned by the thread
typedef struct {
  // these are owned, we have to delete them
  VkQueue graphicsQueue;
  VkCommandPool commandPool;
} wld_ThreadOwnedData;

void wld_set_center_async(   //
    WorldState *pWorldState, //
    const ivec3 centerLoc    //
) {
  // copy our source volume to another array to avoid clobbering it
  Chunk *old[RENDER_DISTANCE_X][RENDER_DISTANCE_Y][RENDER_DISTANCE_Z];
  for (uint32_t x = 0; x < RENDER_DISTANCE_X; x++) {
    for (uint32_t y = 0; y < RENDER_DISTANCE_Y; y++) {
      for (uint32_t z = 0; z < RENDER_DISTANCE_Z; z++) {
        // move
        old[x][y][z] = pWorldState->data[x][y][z];
        pWorldState->data[x][y][z] = NULL;
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
        pWorldState->data[x][y][z] = old[x + disp[0]][y + disp[1]][z + disp[2]];

        // set to null
        old[x + disp[0]][y + disp[1]][z + disp[2]] = NULL;
      }
    }
  }

  // delete all the chunks we weren't able to move
  // unless they are currently pinned, in which case put them in the deadlist
  for (uint32_t x = 0; x < RENDER_DISTANCE_X; x++) {
    for (uint32_t y = 0; y < RENDER_DISTANCE_Y; y++) {
      for (uint32_t z = 0; z < RENDER_DISTANCE_Z; z++) {
        Chunk *outOfRangeChunk = old[x][y][z];
        if (outOfRangeChunk != NULL) {
          if (outOfRangeChunk->state == wld_cs_PINNED) {
            pWorldState->deadchunk_vec[pWorldState->deadchunk_len] =
                outOfRangeChunk;
            pWorldState->deadchunk_len++;
          } else {
            wld_delete_Chunk(outOfRangeChunk, pWorldState->device);
            free(outOfRangeChunk);
          }
        }
      }
    }
  }

  // start a thread to generate all chunks not yet initialized
  for (uint32_t x = 0; x < RENDER_DISTANCE_X; x++) {
    for (uint32_t y = 0; y < RENDER_DISTANCE_Y; y++) {
      for (uint32_t z = 0; z < RENDER_DISTANCE_Z; z++) {
        if (pWorldState->data[x][y][z] == NULL) {
          // calculate chunk offset
          ivec3 worldChunkCoords;
          arrayCoords_to_worldChunkCoords(worldChunkCoords, centerLoc, x, y, z);

          // allocate data
          pWorldState->data[x][y][z] = malloc(sizeof(Chunk));

          // construct data to be sent to task
          wld_WorkerOwnedData *wod = malloc(sizeof(wld_WorkerOwnedData));
          wod->c = pWorldState->data[x][y][z];
          worldChunkCoords_to_blockCoords(wod->chunkOffset, worldChunkCoords);
          wod->noiseCtx = pWorldState->noise1;
          wod->device = pWorldState->device;
          wod->physicalDevice = pWorldState->physicalDevice;
          wod->commandPoolArr = pWorldState->threadCommandPools;
          wod->queue = pWorldState->queue;
          wod->pQueueMutex = pWorldState->pQueueMutex;

          // the thread will take ownership of the worker owned data
          threadpool_error_t err = threadpool_add(
              pWorldState->threadpool, worker_generate_chunk, wod, 0);

          // check for error
          if (err != 0) {
            LOG_ERROR(ERR_LEVEL_FATAL, "threadpool failed to accept task");
            PANIC();
          }
        }
      }
    }
  }

  ivec3_dup(pWorldState->centerLoc, centerLoc);
}
