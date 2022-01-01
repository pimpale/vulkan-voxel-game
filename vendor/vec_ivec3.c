#include <assert.h>
#include <stdlib.h>
#include <vec_ivec3.h>

struct ivec3_vec_s {
  ivec3 *pData;
  uint32_t cap;
  uint32_t len;
};

void new_ivec3_vec(ivec3_vec **pVec) {
  ivec3_vec *vec = malloc(sizeof(ivec3_vec));
  vec->len = 0;
  vec->cap = 16;
  vec->pData = malloc(vec->cap * sizeof(ivec3));
  *pVec = vec;
}

void ivec3_vec_push(ivec3_vec *vec, const ivec3 src) {
  if (vec->len >= vec->cap) {
    vec->cap *= 2;
    vec->pData = realloc(vec->pData, vec->cap * sizeof(ivec3));
  }
  ivec3_dup(vec->pData[vec->len], src);
  vec->len++;
}

void ivec3_vec_pop(ivec3_vec *vec, ivec3 dest) {
  assert(vec->len > 0);
  ivec3_dup(dest, vec->pData[vec->len - 1]);
  vec->len--;
}

void ivec3_vec_clear(ivec3_vec *vec) {
    vec->len = 0;
}

void ivec3_vec_get(const ivec3_vec *vec, uint32_t i, ivec3 dest) {
  assert(i < vec->len);
  ivec3_dup(dest, vec->pData[i]);
}

void ivec3_vec_swapAndPop(ivec3_vec *vec, uint32_t i) {
  assert(vec->len > 0);
  assert(i < vec->len);
  ivec3_dup(vec->pData[i], vec->pData[vec->len - 1]);
  vec->len--;
}

uint32_t ivec3_vec_len(const ivec3_vec *vec) { return vec->len; }

void delete_ivec3_vec(ivec3_vec **pVec) {
  free((*pVec)->pData);
  free(*pVec);
  *pVec = NULL;
}
