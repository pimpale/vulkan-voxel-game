#ifndef SRC_IVEC3_VEC_H_
#define SRC_IVEC3_VEC_H_

#include <stdint.h>
#include <ivec3.h>


typedef struct ivec3_vec_s ivec3_vec;

void new_ivec3_vec(ivec3_vec** vec);

void ivec3_vec_push(ivec3_vec* vec, const ivec3 src);
void ivec3_vec_pop(ivec3_vec* vec, ivec3 dest);

void ivec3_vec_clear(ivec3_vec* vec);

// writes the element at the i'th index to dest
void ivec3_vec_get(const ivec3_vec* vec, uint32_t i, ivec3 dest);

// swaps i with the last element and then pops, deleting the data
void ivec3_vec_swapAndPop(ivec3_vec* vec, uint32_t i);

uint32_t ivec3_vec_len(const ivec3_vec* vec);

void delete_ivec3_vec(ivec3_vec** vec);

#endif
