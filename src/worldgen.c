#include "worldgen.h"

#include <stdlib.h>

#include <open-simplex-noise.h>

// definition of worldgen state
struct worldgen_state_t {
  // noise used to generate more chunks
  struct osn_context *noiseCtx;
};

worldgen_state* new_worldgen_state(uint32_t seed) {
  worldgen_state* pState = malloc(sizeof(worldgen_state));
  // set noise
  open_simplex_noise(seed, &pState->noiseCtx);
  return pState;
}

void delete_worldgen_state(worldgen_state *state) {
  // free simplex noise
  open_simplex_noise_free(state->noiseCtx);
  free(state);
}

// generate chunk data
void worldgen_state_gen_chunk(    //
    ChunkData *pCd,               //
    const ivec3 worldChunkCoords, //
    const worldgen_state *state   //
) {
  // generate chunk, we need to give it the block coordinate to generate at
  vec3 chunkOffset;
  worldChunkCoords_to_blockCoords(chunkOffset, worldChunkCoords);

  double scale1 = 20.0;
  for (uint32_t x = 0; x < CHUNK_X_SIZE; x++) {
    for (uint32_t y = 0; y < CHUNK_Y_SIZE; y++) {
      for (uint32_t z = 0; z < CHUNK_Z_SIZE; z++) {
        // calculate world coordinates in blocks
        double wx = x + (double)chunkOffset[0];
        double wy = y + (double)chunkOffset[1];
        double wz = z + (double)chunkOffset[2];
        double val = open_simplex_noise3(state->noiseCtx, wx / scale1,
                                         wy / scale1, wz / scale1);
        double val2 = open_simplex_noise3(state->noiseCtx, wx / scale1,
                                          (wy - 1) / scale1, wz / scale1);
        if (val > 0 && val2 < 0) {
          pCd->blocks[x][y][z] = 1; // grass
        } else if (val > 0) {
          pCd->blocks[x][y][z] = 2; // grass
        } else {
          pCd->blocks[x][y][z] = 0; // air
        }
      }
    }
  }
}
