#ifndef SRC_WORLD_H_
#define SRC_WORLD_H_

#include <hashmap.h>
#include <stdbool.h>
#include <stdint.h>
#include <vec_ivec3.h>

#include <open-simplex-noise.h>

#include "vulkan_utils.h"

#include "world_utils.h"

// how many chunks to render
#define RENDER_DISTANCE_X 5
#define RENDER_DISTANCE_Y 5
#define RENDER_DISTANCE_Z 5

// max chunks to generate per tick
#define MAX_CHUNKS_TO_GENERATE 10
// max chunks to mesh per tick
#define MAX_CHUNKS_TO_MESH 10

// max chunks to unload per tick
#define MAX_CHUNKS_TO_UNLOAD 10

typedef struct ChunkGeometry_s ChunkGeometry;

/// wld_WorldState
/// ---------------------
/// This struct manages the game world
/// --- THREAD SAFETY ---
/// Do not use this object from more than 1 thread
typedef struct {
  // data in this struct is in general owned by the main thread
  // this means it shouldn't be touched by functions operating in the world
  // thread

  // Chunkspace coordinates
  ivec3 centerLoc;

  // noise used to generate more chunks
  struct osn_context *noise1;

  // these are borrowed, not owned,
  // so make sure you delete world state before deleting these
  VkDevice device;
  VkPhysicalDevice physicalDevice;
  VkQueue queue;
  VkCommandPool commandPool;

  // hashmap storing chunks
  struct hashmap *chunk_map;

  // vector of the coordinates of chunks to generate
  ivec3_vec *togenerate;
  // vector of the coordinates of chunks to mesh
  ivec3_vec *tomesh;
  // vector of the coordinates of ready chunks
  ivec3_vec *ready;
  // vector of the coordinates of chunks to unload
  ivec3_vec *tounload;

  // vector of garbage
  uint32_t garbage_cap;
  uint32_t garbage_len;
  ChunkGeometry** garbage_data;

} WorldState;

/// Creates a new worldState with the given center
void wld_new_WorldState(                  //
    WorldState *pWorldState,              //
    const ivec3 centerLoc,                //
    const uint32_t seed,                  //
    const VkQueue queue,                  //
    const VkCommandPool commandPool,      //
    const VkDevice device,                //
    const VkPhysicalDevice physicalDevice //
);

/// --- PRECONDITIONS ---
/// --- POSTCONDITIONS ---
/// * state is UNINITIALIZED
void wld_delete_WorldState( //
    WorldState *pWorldState //
);

/// --- PRECONDITIONS ---
/// * pWorldState is valid
///--- POSTCONDITIONS ---
/// * if state is UNAVAILABLE returns false
/// * if state is AVAILABLE, returns true and starts working
void wld_set_center(   //
    WorldState *pWorldState, //
    const ivec3 centerLoc    //
);

/// updates the world
void wld_update( //
    WorldState *pWorldState       //
);

/// gets rid of the garbage buffers, make sure none of it is in use
void wld_clearGarbage(WorldState *pWorldState);

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
