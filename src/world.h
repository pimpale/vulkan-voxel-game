#ifndef SRC_WORLD_H_
#define SRC_WORLD_H_

#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

#include <open-simplex-noise.h>
#include <rpa_queue.h>

#include "vulkan_utils.h"

#include "world_utils.h"

// how many chunks to render
#define RENDER_DISTANCE_X 5
#define RENDER_DISTANCE_Y 5
#define RENDER_DISTANCE_Z 5

typedef struct ChunkGeometry_s ChunkGeometry;

// this data is owned by the thread
typedef struct {
  // Chunkspace coordinates
  ivec3 centerLoc;

  // noise used to generate more chunks
  struct osn_context *noise1;

  // these are borrowed, not owned, 
  // so make sure you delete world state before deleting these
  VkDevice device;
  VkPhysicalDevice physicalDevice;

  // these are owned, we have to delete them
  VkQueue graphicsQueue;
  VkCommandPool commandPool;

  // the RENDER_DISTANCE_X/2, RENDER_DISTANCE_Y/2, RENDER_DISTANCE_Z/2'th block
  // is our centerLoc
  ChunkData *blocks[RENDER_DISTANCE_X][RENDER_DISTANCE_Y][RENDER_DISTANCE_Z];

  // dont access without locking the mutex
  pthread_mutex_t geometry_mutex;
  ChunkGeometry *geometry[RENDER_DISTANCE_X][RENDER_DISTANCE_Y][RENDER_DISTANCE_Z];

} wld_ThreadOwnedData;

/// wld_WorldState
/// ---------------------
/// This struct manages the game world
/// It has seperate internal threads that do asynchronous work
/// The threads are owned by wld_WorldState, and must not be accessed externally
/// --- STATES ---
/// * UNINITIALIZED: worldstate is uninitialized, or has been freed
/// * AVAILABLE: worldstate is ready to accept commands
/// * UNAVAILABLE: worldstate is busy, and will drop any commands.
/// --- THREAD SAFETY ---
/// Do not use this object from more than 1 thread
typedef struct {
  // data in this struct is in general owned by the main thread
  // this means it shouldn't be touched by functions operating in the world thread

  // thread owned data, don't touch directly
  wld_ThreadOwnedData tod;

  // both threads can access this queue
  // its meant for sending data from main to world thread
  rpa_queue_t* toThread;

  // cached chunk geometry
  ChunkGeometry *cachedGeometry[RENDER_DISTANCE_X][RENDER_DISTANCE_Y][RENDER_DISTANCE_Z];

  // Chunkspace coordinates
  ivec3 centerLoc;
} WorldState;

/// Synchronously creates a new worldState and awaits setting the center
void wld_new_WorldState(                  //
    WorldState *pWorldState,              //
    const ivec3 centerLoc,                //
    const uint32_t seed,                  //
    const uint32_t graphicsQueueFamily,   //
    const VkDevice device,                //
    const VkPhysicalDevice physicalDevice //
);

/// --- PRECONDITIONS ---
/// * state must be AVAILABLE or UNAVAILABLE
/// --- POSTCONDITIONS ---
/// * state is UNINITIALIZED
void wld_delete_WorldState( //
    WorldState *pWorldState //
);

/// --- PRECONDITIONS ---
/// * pWorldState is valid
/// * state is AVAILABLE or UNAVAILABLE
///--- POSTCONDITIONS ---
/// * if state is UNAVAILABLE returns false
/// * if state is AVAILABLE, returns true and starts working
bool wld_set_center_async(         //
    WorldState *pWorldState, //
    const ivec3 centerLoc    //
);

/// by default, when we do some work, the changes are done to a second buffer
/// this prevents issues with data being mid-update when accessed
/// This method allows our background work to be brought into the foreground.
/// Remember to call it before 
/// First, ensure t
bool wld_showGeometryUpdates_async(     //
    uint32_t *pVertexBufferCount, //
    const WorldState *pWorldState //
);

/// returns the number of vertex buffers
/// this number won't change unless you call wld_showGeometryUpdates
/// --- PRECONDITIONS ---
/// * pVertexBufferCount is valid
/// * pWorldState is valid
/// * state is AVAILABLE or UNAVAILABLE
/// --- POSTCONDITIONS ---
/// * sets `*pVertexBufferCount` to the current number of vertex buffers
void wld_count_vertexBuffers(     //
    uint32_t *pVertexBufferCount, //
    const WorldState *pWorldState //
);

/// writes the vertex buffers and vertex counts to a set of arrays
/// this data won't change unless you call wld_showGeometryUpdates
void wld_getVertexBuffers(        //
    VkBuffer *pVertexBuffers,     //
    uint32_t *pVertexCounts,      //
    const WorldState *pWorldState //
);

#endif
