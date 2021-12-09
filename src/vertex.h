#ifndef SRC_VERTEX_H_
#define SRC_VERTEX_H_

#include <linmath.h>
#include <stdint.h>

typedef struct {
  vec3 position;
  vec3 color;
} Vertex;

typedef uint32_t uvec3[3];

static inline void uvec3_dup(uvec3 d, const uvec3 a) {
  d[0] = a[0];
  d[1] = a[1];
  d[2] = a[2];
}

static inline void uvec3_scale(uvec3 d, const uvec3 a, const float s) {
  d[0] = (uint32_t)((float)a[0] * s);
  d[1] = (uint32_t)((float)a[1] * s);
  d[2] = (uint32_t)((float)a[2] * s);
}

static inline void uvec3_add(uvec3 d, const uvec3 a, const uvec3 b) {
  d[0] = a[0] + b[0];
  d[1] = a[1] + b[1];
  d[2] = a[2] + b[2];
}

static inline void uvec3_sub(uvec3 d, const uvec3 a, const uvec3 b) {
  d[0] = a[0] - b[0];
  d[1] = a[1] - b[1];
  d[2] = a[2] - b[2];
}

static inline void uvec3_conv(vec3 d, const uvec3 a) {
  d[0] = (float)a[0];
  d[1] = (float)a[1];
  d[2] = (float)a[2];
}

typedef int32_t ivec3[3];

static inline void ivec3_dup(ivec3 d, const ivec3 a) {
  d[0] = a[0];
  d[1] = a[1];
  d[2] = a[2];
}

static inline void ivec3_scale(ivec3 d, const ivec3 a, const float s) {
  d[0] = (int32_t)((float)a[0] * s);
  d[1] = (int32_t)((float)a[1] * s);
  d[2] = (int32_t)((float)a[2] * s);
}

static inline void ivec3_add(ivec3 d, const ivec3 a, const ivec3 b) {
  d[0] = a[0] + b[0];
  d[1] = a[1] + b[1];
  d[2] = a[2] + b[2];
}

static inline void ivec3_sub(ivec3 d, const ivec3 a, const ivec3 b) {
  d[0] = a[0] - b[0];
  d[1] = a[1] - b[1];
  d[2] = a[2] - b[2];
}

static inline void ivec3_conv(vec3 d, const ivec3 a) {
  d[0] = (float)a[0];
  d[1] = (float)a[1];
  d[2] = (float)a[2];
}

#endif
