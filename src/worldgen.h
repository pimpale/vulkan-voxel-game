#ifndef SRC_WORLDGEN_H_
#define SRC_WORLDGEN_H_

#include <stdbool.h>
#include <stdint.h>

#include <open-simplex-noise.h>

#include "vertex.h"

// Size of chunk in blocks
#define CHUNK_SIZE 32

// how many chunks to render
#define RENDER_DISTANCE_X 5
#define RENDER_DISTANCE_Y 5
#define RENDER_DISTANCE_Z 5

typedef struct {
  bool transparent;
} Block;

typedef struct {
  Block blocks[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];
} Chunk;

typedef struct {
  // noise used to generate more chunks
  struct osn_context *noise1;

  // Chunkspace coordinates
  ivec3 centerLoc;
  // the RENDER_DISTANCE_X/2, RENDER_DISTANCE_Y/2, RENDER_DISTANCE_Z/2'th block
  // is our centerLoc
  Chunk *local[RENDER_DISTANCE_X][RENDER_DISTANCE_Y][RENDER_DISTANCE_Z];
} WorldState;

void wg_new_WorldState_(     //
    WorldState *pWorldState, //
    const ivec3 centerLoc,   //
    const uint32_t seed      //
);

void wg_translate(           //
    WorldState *pWorldState, //
    const ivec3 centerLoc    //
);

void wg_world_count_vertexes(     //
    uint32_t *pVertexCount,       //
    const WorldState *pWorldState //
);

void wg_world_mesh(               //
    Vertex *pVertexes,            //
    const vec3 offset,  //
    const WorldState *pWorldState //
);

#endif
