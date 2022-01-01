#ifndef SRC_VERTEX_H_
#define SRC_VERTEX_H_

#include <linmath.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
  vec3 position;
  vec3 normal;
  vec2 texCoords;
} Vertex;

#endif
