#ifndef WORLDGEN_H
#define WORLDGEN_H

#include <stdint.h>

#include "world_utils.h"

typedef struct worldgen_state_t worldgen_state;

worldgen_state* new_worldgen_state(uint32_t seed);

void worldgen_state_gen_chunk(ChunkData *pCd, const ivec3 chunkOffset, const worldgen_state* state);

void delete_worldgen_state(worldgen_state* state);

#endif

