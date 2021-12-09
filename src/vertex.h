#ifndef SRC_VERTEX_H_
#define SRC_VERTEX_H_

#include <linmath.h>
#include <stdint.h>

typedef struct {
  vec3 position;
  vec3 color;
} Vertex;

typedef struct {
  uint32_t x;
  uint32_t y;
  uint32_t z;
} uvec3;


typedef struct {
  int32_t x;
  int32_t y;
  int32_t z;
} ivec3;


#endif
