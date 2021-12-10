#ifndef SRC_WORLD_H_
#define SRC_WORLD_H_

#include <stdbool.h>
#include <stdint.h>

#include <open-simplex-noise.h>

#include "vulkan_utils.h"

// Size of chunk in blocks
#define CHUNK_X_SIZE 32
#define CHUNK_Y_SIZE 32
#define CHUNK_Z_SIZE 32

// how many chunks to render
#define RENDER_DISTANCE_X 5
#define RENDER_DISTANCE_Y 5
#define RENDER_DISTANCE_Z 5

typedef struct Chunk_s Chunk;

typedef struct {
  // noise used to generate more chunks
  struct osn_context *noise1;

  // Chunkspace coordinates
  ivec3 centerLoc;
  // the RENDER_DISTANCE_X/2, RENDER_DISTANCE_Y/2, RENDER_DISTANCE_Z/2'th block
  // is our centerLoc
  Chunk *local[RENDER_DISTANCE_X][RENDER_DISTANCE_Y][RENDER_DISTANCE_Z];
} WorldState;

void wld_new_WorldState(                   //
    WorldState *pWorldState,               //
    const ivec3 centerLoc,                 //
    const uint32_t seed,                   //
    const VkDevice device,                 //
    const VkPhysicalDevice physicalDevice, //
    const VkCommandPool commandPool,       //
    const VkQueue graphicsQueue            //
);

void wld_delete_WorldState(  //
    WorldState *pWorldState, //
    const VkDevice device    //
);

void wld_set_center(                       //
    WorldState *pWorldState,               //
    const ivec3 centerLoc,                 //
    const VkDevice device,                 //
    const VkPhysicalDevice physicalDevice, //
    const VkCommandPool commandPool,       //
    const VkQueue graphicsQueue            //

);

void wld_count_vertexes(          //
    uint32_t *pVertexCount,       //
    const WorldState *pWorldState //
);

void wld_count_vertexBuffers(     //
    uint32_t *pVertexBufferCount, //
    const WorldState *pWorldState //
);

void wld_getVertexBuffers(        //
    VkBuffer *pVertexBuffers,     //
    const WorldState *pWorldState //
);

void wld_blockCoord_to_worldChunkCoords(   //
    ivec3 chunkCoord,     //
    const vec3 blockCoord //
);

bool wld_centered(                 //
    const WorldState *pWorldState, //
    const ivec3 blockCoord         //
);

#endif
