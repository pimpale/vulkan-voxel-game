#include "world_utils.h"

#include <stdio.h>

// converts from world chunk coordinates to global block coordinates
void worldChunkCoords_to_iBlockCoords( //
    ivec3 iBlockCoords,                //
    const ivec3 worldChunkCoords       //
) {
  iBlockCoords[0] = worldChunkCoords[0] * CHUNK_X_SIZE;
  iBlockCoords[1] = worldChunkCoords[1] * CHUNK_Y_SIZE;
  iBlockCoords[2] = worldChunkCoords[2] * CHUNK_Z_SIZE;
}

void blockCoords_to_worldChunkCoords( //
    ivec3 chunkCoord,                 //
    const vec3 blockCoord             //
) {
  chunkCoord[0] = (int32_t)(floorf(blockCoord[0] / CHUNK_X_SIZE));
  chunkCoord[1] = (int32_t)(floorf(blockCoord[1] / CHUNK_Y_SIZE));
  chunkCoord[2] = (int32_t)(floorf(blockCoord[2] / CHUNK_Z_SIZE));
}

// converts from world chunk coordinates to global block coordinates
void worldChunkCoords_to_blockCoords( //
    vec3 blockCoords,                 //
    const ivec3 worldChunkCoords      //
) {
  ivec3 tmp;
  worldChunkCoords_to_iBlockCoords(tmp, worldChunkCoords);
  ivec3_to_vec3(blockCoords, tmp);
}

bool wu_loadChunkData(ChunkData *pC, const char *filename) {
  FILE *file = fopen(filename, "rb");
  if (!file) {
    return false;
  }

  // read whole data at once
  size_t chunk_bytes = CHUNK_X_SIZE * CHUNK_Y_SIZE * CHUNK_Z_SIZE;

  // read data at once
  size_t data_read_bytes = fread(pC->blocks, 1, chunk_bytes, file);

  fclose(file);

  if (data_read_bytes != chunk_bytes) {
    return false;
  }

  return true;
}

uint32_t wu_countChunkDataVertexes( //
    const ChunkData *pCd            //
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
uint32_t wu_getVertexesChunkData( //
    Vertex *pVertexes,            //
    const vec3 offset,            //
    const ChunkData *pCd          //
) {
  uint32_t i = 0;
  for (uint32_t x = 0; x < CHUNK_X_SIZE; x++) {
    for (uint32_t y = 0; y < CHUNK_Y_SIZE; y++) {
      for (uint32_t z = 0; z < CHUNK_Z_SIZE; z++) {
        BlockIndex bi = pCd->blocks[x][y][z];
        // check that its not transparent
        if (BLOCKS[bi].transparent) {
          continue;
        }

        // get chunk location
        const float fx = (float)x + offset[0];
        const float fy = (float)y + offset[1];
        const float fz = (float)z + offset[2];

        // calculate vertexes
        const vec3 v000 = {fx + 0, fy + 0, fz + 0};
        const vec3 v100 = {fx + 1, fy + 0, fz + 0};
        const vec3 v001 = {fx + 0, fy + 0, fz + 1};
        const vec3 v101 = {fx + 1, fy + 0, fz + 1};
        const vec3 v010 = {fx + 0, fy + 1, fz + 0};
        const vec3 v110 = {fx + 1, fy + 1, fz + 0};
        const vec3 v011 = {fx + 0, fy + 1, fz + 1};
        const vec3 v111 = {fx + 1, fy + 1, fz + 1};

        const float xoff = BLOCK_TILE_TEX_XSIZE;
        const float yoff = BLOCK_TILE_TEX_YSIZE;

        // clang-format off

        // left face
        if (x == 0 || BLOCKS[pCd->blocks[x - 1][y][z]].transparent) {
          const float bx = BLOCK_TILE_TEX_XSIZE*Block_LEFT;
          const float by = BLOCK_TILE_TEX_YSIZE*bi;
          pVertexes[i++] = (Vertex){.position=V3(v000), .normal={-1.0f, 0.0f, 0.0f}, .texCoords= {bx+0.0f, by+0.0f}};
          pVertexes[i++] = (Vertex){.position=V3(v010), .normal={-1.0f, 0.0f, 0.0f}, .texCoords= {bx+0.0f, by+yoff}};
          pVertexes[i++] = (Vertex){.position=V3(v001), .normal={-1.0f, 0.0f, 0.0f}, .texCoords= {bx+xoff, by+0.0f}};
          pVertexes[i++] = (Vertex){.position=V3(v001), .normal={-1.0f, 0.0f, 0.0f}, .texCoords= {bx+xoff, by+0.0f}};
          pVertexes[i++] = (Vertex){.position=V3(v010), .normal={-1.0f, 0.0f, 0.0f}, .texCoords= {bx+0.0f, by+yoff}};
          pVertexes[i++] = (Vertex){.position=V3(v011), .normal={-1.0f, 0.0f, 0.0f}, .texCoords= {bx+xoff, by+yoff}};
        }
        // right face
        if (x == CHUNK_X_SIZE - 1 ||
            BLOCKS[pCd->blocks[x + 1][y][z]].transparent) {
          const float bx = BLOCK_TILE_TEX_XSIZE*Block_RIGHT;
          const float by = BLOCK_TILE_TEX_YSIZE*bi;
          pVertexes[i++] = (Vertex){.position=V3(v100), .normal={1.0f, 0.0f, 0.0f}, .texCoords= {bx+xoff, by+0.0f}};
          pVertexes[i++] = (Vertex){.position=V3(v101), .normal={1.0f, 0.0f, 0.0f}, .texCoords= {bx+0.0f, by+0.0f}};
          pVertexes[i++] = (Vertex){.position=V3(v110), .normal={1.0f, 0.0f, 0.0f}, .texCoords= {bx+xoff, by+yoff}};
          pVertexes[i++] = (Vertex){.position=V3(v101), .normal={1.0f, 0.0f, 0.0f}, .texCoords= {bx+0.0f, by+0.0f}};
          pVertexes[i++] = (Vertex){.position=V3(v111), .normal={1.0f, 0.0f, 0.0f}, .texCoords= {bx+0.0f, by+yoff}};
          pVertexes[i++] = (Vertex){.position=V3(v110), .normal={1.0f, 0.0f, 0.0f}, .texCoords= {bx+xoff, by+yoff}};
        }

        // upper face
        if (y == 0 || BLOCKS[pCd->blocks[x][y - 1][z]].transparent) {
          const float bx = BLOCK_TILE_TEX_XSIZE*Block_UP;
          const float by = BLOCK_TILE_TEX_YSIZE*bi;
          pVertexes[i++] = (Vertex){.position=V3(v001), .normal={0.0f, -1.0f, 0.0f}, .texCoords={bx+0.0f, by+yoff}};
          pVertexes[i++] = (Vertex){.position=V3(v100), .normal={0.0f, -1.0f, 0.0f}, .texCoords={bx+xoff, by+0.0f}};
          pVertexes[i++] = (Vertex){.position=V3(v000), .normal={0.0f, -1.0f, 0.0f}, .texCoords={bx+0.0f, by+0.0f}};
          pVertexes[i++] = (Vertex){.position=V3(v001), .normal={0.0f, -1.0f, 0.0f}, .texCoords={bx+0.0f, by+yoff}};
          pVertexes[i++] = (Vertex){.position=V3(v101), .normal={0.0f, -1.0f, 0.0f}, .texCoords={bx+xoff, by+yoff}};
          pVertexes[i++] = (Vertex){.position=V3(v100), .normal={0.0f, -1.0f, 0.0f}, .texCoords={bx+xoff, by+0.0f}};
        }
        // lower face
        if (y == CHUNK_Y_SIZE - 1 ||
            BLOCKS[pCd->blocks[x][y + 1][z]].transparent) {
          const float bx = BLOCK_TILE_TEX_XSIZE*Block_DOWN;
          const float by = BLOCK_TILE_TEX_YSIZE*bi;
          pVertexes[i++] = (Vertex){.position=V3(v010), .normal={0.0f, 1.0f, 0.0f}, .texCoords={bx+xoff, by+0.0f}};
          pVertexes[i++] = (Vertex){.position=V3(v110), .normal={0.0f, 1.0f, 0.0f}, .texCoords={bx+0.0f, by+0.0f}};
          pVertexes[i++] = (Vertex){.position=V3(v011), .normal={0.0f, 1.0f, 0.0f}, .texCoords={bx+xoff, by+yoff}};
          pVertexes[i++] = (Vertex){.position=V3(v110), .normal={0.0f, 1.0f, 0.0f}, .texCoords={bx+0.0f, by+0.0f}};
          pVertexes[i++] = (Vertex){.position=V3(v111), .normal={0.0f, 1.0f, 0.0f}, .texCoords={bx+0.0f, by+yoff}};
          pVertexes[i++] = (Vertex){.position=V3(v011), .normal={0.0f, 1.0f, 0.0f}, .texCoords={bx+xoff, by+yoff}};
        }

        // back face
        if (z == 0 || BLOCKS[pCd->blocks[x][y][z - 1]].transparent) {
          const float bx = BLOCK_TILE_TEX_XSIZE*Block_BACK;
          const float by = BLOCK_TILE_TEX_YSIZE*bi;
          pVertexes[i++] = (Vertex){.position=V3(v000), .normal={0.0f, 0.0f, -1.0f}, .texCoords={bx+xoff, by+0.0f}};
          pVertexes[i++] = (Vertex){.position=V3(v100), .normal={0.0f, 0.0f, -1.0f}, .texCoords={bx+0.0f, by+0.0f}};
          pVertexes[i++] = (Vertex){.position=V3(v010), .normal={0.0f, 0.0f, -1.0f}, .texCoords={bx+xoff, by+yoff}};
          pVertexes[i++] = (Vertex){.position=V3(v100), .normal={0.0f, 0.0f, -1.0f}, .texCoords={bx+0.0f, by+0.0f}};
          pVertexes[i++] = (Vertex){.position=V3(v110), .normal={0.0f, 0.0f, -1.0f}, .texCoords={bx+0.0f, by+yoff}};
          pVertexes[i++] = (Vertex){.position=V3(v010), .normal={0.0f, 0.0f, -1.0f}, .texCoords={bx+xoff, by+yoff}};
        }

        // front face
        if (z == CHUNK_Z_SIZE - 1 ||
            BLOCKS[pCd->blocks[x][y][z + 1]].transparent) {
          const float bx = BLOCK_TILE_TEX_XSIZE*Block_FRONT;
          const float by = BLOCK_TILE_TEX_YSIZE*bi;
          pVertexes[i++] = (Vertex){.position=V3(v011), .normal={0.0f, 0.0f, 1.0f}, .texCoords={bx+0.0f, by+yoff}};
          pVertexes[i++] = (Vertex){.position=V3(v101), .normal={0.0f, 0.0f, 1.0f}, .texCoords={bx+xoff, by+0.0f}};
          pVertexes[i++] = (Vertex){.position=V3(v001), .normal={0.0f, 0.0f, 1.0f}, .texCoords={bx+0.0f, by+0.0f}};
          pVertexes[i++] = (Vertex){.position=V3(v011), .normal={0.0f, 0.0f, 1.0f}, .texCoords={bx+0.0f, by+yoff}};
          pVertexes[i++] = (Vertex){.position=V3(v111), .normal={0.0f, 0.0f, 1.0f}, .texCoords={bx+xoff, by+yoff}};
          pVertexes[i++] = (Vertex){.position=V3(v101), .normal={0.0f, 0.0f, 1.0f}, .texCoords={bx+xoff, by+0.0f}};
        }
        // clang-format on
      }
    }
  }
  return i;
}

// returns the number of vertexes written
void wu_getVertexesHighlight( //
    Vertex pVertexes[6],      //
    const ivec3 iBlockCoords, //
    const BlockFaceKind face  //
) {

  BlockIndex bi = 0;

  // get chunk location
  const float fx = (float)iBlockCoords[0];
  const float fy = (float)iBlockCoords[1];
  const float fz = (float)iBlockCoords[2];

  // calculate vertexes
  const vec3 v000 = {fx + 0, fy + 0, fz + 0};
  const vec3 v100 = {fx + 1, fy + 0, fz + 0};
  const vec3 v001 = {fx + 0, fy + 0, fz + 1};
  const vec3 v101 = {fx + 1, fy + 0, fz + 1};
  const vec3 v010 = {fx + 0, fy + 1, fz + 0};
  const vec3 v110 = {fx + 1, fy + 1, fz + 0};
  const vec3 v011 = {fx + 0, fy + 1, fz + 1};
  const vec3 v111 = {fx + 1, fy + 1, fz + 1};

  const float xoff = BLOCK_TILE_TEX_XSIZE;
  const float yoff = BLOCK_TILE_TEX_YSIZE;

  // clang-format off
  switch(face) {
  case Block_LEFT: {
    const float bx = BLOCK_TILE_TEX_XSIZE*Block_LEFT;
    const float by = BLOCK_TILE_TEX_YSIZE*bi;
    pVertexes[0] = (Vertex){.position=V3(v000), .normal={-1.0f, 0.0f, 0.0f}, .texCoords= {bx+0.0f, by+0.0f}};
    pVertexes[1] = (Vertex){.position=V3(v010), .normal={-1.0f, 0.0f, 0.0f}, .texCoords= {bx+0.0f, by+yoff}};
    pVertexes[2] = (Vertex){.position=V3(v001), .normal={-1.0f, 0.0f, 0.0f}, .texCoords= {bx+xoff, by+0.0f}};
    pVertexes[3] = (Vertex){.position=V3(v001), .normal={-1.0f, 0.0f, 0.0f}, .texCoords= {bx+xoff, by+0.0f}};
    pVertexes[4] = (Vertex){.position=V3(v010), .normal={-1.0f, 0.0f, 0.0f}, .texCoords= {bx+0.0f, by+yoff}};
    pVertexes[5] = (Vertex){.position=V3(v011), .normal={-1.0f, 0.0f, 0.0f}, .texCoords= {bx+xoff, by+yoff}};
    break;
  }
  case Block_RIGHT: {
    const float bx = BLOCK_TILE_TEX_XSIZE*Block_RIGHT;
    const float by = BLOCK_TILE_TEX_YSIZE*bi;
    pVertexes[0] = (Vertex){.position=V3(v100), .normal={1.0f, 0.0f, 0.0f}, .texCoords= {bx+xoff, by+0.0f}};
    pVertexes[1] = (Vertex){.position=V3(v101), .normal={1.0f, 0.0f, 0.0f}, .texCoords= {bx+0.0f, by+0.0f}};
    pVertexes[2] = (Vertex){.position=V3(v110), .normal={1.0f, 0.0f, 0.0f}, .texCoords= {bx+xoff, by+yoff}};
    pVertexes[3] = (Vertex){.position=V3(v101), .normal={1.0f, 0.0f, 0.0f}, .texCoords= {bx+0.0f, by+0.0f}};
    pVertexes[4] = (Vertex){.position=V3(v111), .normal={1.0f, 0.0f, 0.0f}, .texCoords= {bx+0.0f, by+yoff}};
    pVertexes[5] = (Vertex){.position=V3(v110), .normal={1.0f, 0.0f, 0.0f}, .texCoords= {bx+xoff, by+yoff}};
    break;
  }
  case Block_UP: {
    const float bx = BLOCK_TILE_TEX_XSIZE*Block_UP;
    const float by = BLOCK_TILE_TEX_YSIZE*bi;
    pVertexes[0] = (Vertex){.position=V3(v001), .normal={0.0f, -1.0f, 0.0f}, .texCoords={bx+0.0f, by+yoff}};
    pVertexes[1] = (Vertex){.position=V3(v100), .normal={0.0f, -1.0f, 0.0f}, .texCoords={bx+xoff, by+0.0f}};
    pVertexes[2] = (Vertex){.position=V3(v000), .normal={0.0f, -1.0f, 0.0f}, .texCoords={bx+0.0f, by+0.0f}};
    pVertexes[3] = (Vertex){.position=V3(v001), .normal={0.0f, -1.0f, 0.0f}, .texCoords={bx+0.0f, by+yoff}};
    pVertexes[4] = (Vertex){.position=V3(v101), .normal={0.0f, -1.0f, 0.0f}, .texCoords={bx+xoff, by+yoff}};
    pVertexes[5] = (Vertex){.position=V3(v100), .normal={0.0f, -1.0f, 0.0f}, .texCoords={bx+xoff, by+0.0f}};
    break;
  }
  case Block_DOWN: {
    const float bx = BLOCK_TILE_TEX_XSIZE*Block_DOWN;
    const float by = BLOCK_TILE_TEX_YSIZE*bi;
    pVertexes[0] = (Vertex){.position=V3(v010), .normal={0.0f, 1.0f, 0.0f}, .texCoords={bx+xoff, by+0.0f}};
    pVertexes[1] = (Vertex){.position=V3(v110), .normal={0.0f, 1.0f, 0.0f}, .texCoords={bx+0.0f, by+0.0f}};
    pVertexes[2] = (Vertex){.position=V3(v011), .normal={0.0f, 1.0f, 0.0f}, .texCoords={bx+xoff, by+yoff}};
    pVertexes[3] = (Vertex){.position=V3(v110), .normal={0.0f, 1.0f, 0.0f}, .texCoords={bx+0.0f, by+0.0f}};
    pVertexes[4] = (Vertex){.position=V3(v111), .normal={0.0f, 1.0f, 0.0f}, .texCoords={bx+0.0f, by+yoff}};
    pVertexes[5] = (Vertex){.position=V3(v011), .normal={0.0f, 1.0f, 0.0f}, .texCoords={bx+xoff, by+yoff}};
    break;
  }
  case Block_BACK: {
    const float bx = BLOCK_TILE_TEX_XSIZE*Block_BACK;
    const float by = BLOCK_TILE_TEX_YSIZE*bi;
    pVertexes[0] = (Vertex){.position=V3(v000), .normal={0.0f, 0.0f, -1.0f}, .texCoords={bx+xoff, by+0.0f}};
    pVertexes[1] = (Vertex){.position=V3(v100), .normal={0.0f, 0.0f, -1.0f}, .texCoords={bx+0.0f, by+0.0f}};
    pVertexes[2] = (Vertex){.position=V3(v010), .normal={0.0f, 0.0f, -1.0f}, .texCoords={bx+xoff, by+yoff}};
    pVertexes[3] = (Vertex){.position=V3(v100), .normal={0.0f, 0.0f, -1.0f}, .texCoords={bx+0.0f, by+0.0f}};
    pVertexes[4] = (Vertex){.position=V3(v110), .normal={0.0f, 0.0f, -1.0f}, .texCoords={bx+0.0f, by+yoff}};
    pVertexes[5] = (Vertex){.position=V3(v010), .normal={0.0f, 0.0f, -1.0f}, .texCoords={bx+xoff, by+yoff}};
    break;
  }
  case Block_FRONT: {
    const float bx = BLOCK_TILE_TEX_XSIZE*Block_FRONT;
    const float by = BLOCK_TILE_TEX_YSIZE*bi;
    pVertexes[0] = (Vertex){.position=V3(v011), .normal={0.0f, 0.0f, 1.0f}, .texCoords={bx+0.0f, by+yoff}};
    pVertexes[1] = (Vertex){.position=V3(v101), .normal={0.0f, 0.0f, 1.0f}, .texCoords={bx+xoff, by+0.0f}};
    pVertexes[2] = (Vertex){.position=V3(v001), .normal={0.0f, 0.0f, 1.0f}, .texCoords={bx+0.0f, by+0.0f}};
    pVertexes[3] = (Vertex){.position=V3(v011), .normal={0.0f, 0.0f, 1.0f}, .texCoords={bx+0.0f, by+yoff}};
    pVertexes[4] = (Vertex){.position=V3(v111), .normal={0.0f, 0.0f, 1.0f}, .texCoords={bx+xoff, by+yoff}};
    pVertexes[5] = (Vertex){.position=V3(v101), .normal={0.0f, 0.0f, 1.0f}, .texCoords={bx+xoff, by+0.0f}};
    break;
  }
  }
  // clang-format on
}

void wu_getAdjacentBlock(        //
    ivec3 destiBlockCoords,      //
    const ivec3 srciBlockCoords, //
    const BlockFaceKind face     //
) {
  switch (face) {
  case Block_LEFT: {
    ivec3 normal = {-1.0f, 0.0f, 0.0f};
    ivec3_add(destiBlockCoords, srciBlockCoords, normal);
    break;
  }
  case Block_RIGHT: {
    ivec3 normal = {1.0f, 0.0f, 0.0f};
    ivec3_add(destiBlockCoords, srciBlockCoords, normal);
    break;
  }
  case Block_UP: {
    ivec3 normal = {0.0f, -1.0f, 0.0f};
    ivec3_add(destiBlockCoords, srciBlockCoords, normal);
    break;
  }
  case Block_DOWN: {
    ivec3 normal = {0.0f, 1.0f, 0.0f};
    ivec3_add(destiBlockCoords, srciBlockCoords, normal);
    break;
  }
  case Block_BACK: {
    ivec3 normal = {0.0f, 0.0f, -1.0f};
    ivec3_add(destiBlockCoords, srciBlockCoords, normal);
    break;
  }
  case Block_FRONT: {
    ivec3 normal = {0.0f, 0.0f, 1.0f};
    ivec3_add(destiBlockCoords, srciBlockCoords, normal);
    break;
  }
  }
}
