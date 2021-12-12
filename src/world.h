#ifndef SRC_WORLD_H_
#define SRC_WORLD_H_

#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>
#include <threadpool.h>

#include <open-simplex-noise.h>

#include "vulkan_utils.h"

#include "world_utils.h"

// how many chunks to render
#define RENDER_DISTANCE_X 5
#define RENDER_DISTANCE_Y 5
#define RENDER_DISTANCE_Z 5

// How many threads to render with
#define RENDER_THREADS 5

// idea
// use a threadpool (stored in the main thread)
// for chunks to be deleted, get rid of them immediately, 
// unless they are state PINNED, in which case put the buffers and memory in the pinned stack
// unless they are state RENDERING, in which case, put them in the pinnned buffer
// for each new chunk to add, immediately start the task working on it.
// The task will have a pointer to our chunk, which it will update
// once it is done, the task will return a malloced pointer containing the new data

// if we get asecond update to the chunk

// when we call wld_doGeometryUpdates_async
// First, get rid of all the stuff in the pinned bufferm we don't need it anymore
// iterate through our chunk array and copy paste all of vertex buffers and count from chunks in state READY, to the vertex buffer count and the vertex buffer cache in main thread
// change those states to PINNED

// when we get normal requests, just hand over data from the cache arrays


typedef enum {
    // the blocks need to be generated
    wld_cs_NEEDS_GENERATE,
    // the world generation process is going asynchronously,
    wld_cs_GENERATING,
    // the world generation or block update is done, and the chunk is empty
    wld_cs_EMPTY,
    // the world generation finished, or we just updated the block manually
    wld_cs_NEEDS_RENDER,
    // the chunk is rendering asynchronously
    wld_cs_RENDERING,
    // the chunk is finished rendering and is read to display
    wld_cs_READY,
    // the chunk is in use in the world
    wld_cs_PINNED
} wld_ChunkState ;


typedef struct {
  uint32_t vertexCount;
  VkBuffer vertexBuffer;
  VkDeviceMemory vertexBufferMemory;
} ChunkGeometry;


typedef struct {
  // this variable will only be changed from the worker thread
  volatile wld_ChunkState state;

  // this variable will only be changed form the worker thread
  volatile bool working;

  // this variable will only be changed from the main thread
  // it tells the thread to quit working, the chunk has stopped being rendered
  volatile bool chunk_dead;

  // block data
  ChunkData chunkData;

  // mesh data
  ChunkGeometry geometry;
} Chunk;

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

  // Chunkspace coordinates
  ivec3 centerLoc;

  // noise used to generate more chunks
  struct osn_context *noise1;

  // these are borrowed, not owned, 
  // so make sure you delete world state before deleting these
  VkDevice device;
  VkPhysicalDevice physicalDevice;

  // different threads get different slices of z
  Chunk *data[RENDER_DISTANCE_X][RENDER_DISTANCE_Y][RENDER_DISTANCE_Z];

  // contains threads
  threadpool_t* threadpool;

  // each worker thread gets its own command pool
  VkCommandPool threadCommandPools[RENDER_THREADS];
  // each worker thread gets its own submit queue
  VkCommandPool threadQueues[RENDER_THREADS];

  // vector of pinned chunk geometries (these are chunks that are being used by the main thread)
  size_t pinned_capacity;
  size_t pinned_len;
  ChunkGeometry** pinned_vec;

  // vector of dead chunk geometries (this is used for chunks that are now obsolete, but the main thread might still be using)
  size_t deadchunk_capacity;
  size_t deadchunk_len;
  ChunkGeometry** deadchunk_vec;

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
bool wld_doGeometryUpdates_async(     //
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
