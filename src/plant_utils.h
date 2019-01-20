/* Copyright 2019 Govind Pimpale
 * plant_utils.h
 *
 *  Created on: Jan 16, 2019
 *      Author: gpi
 */

#ifndef SRC_PLANT_UTILS_H_
#define SRC_PLANT_UTILS_H_

#include "linmath.h"

typedef struct {
  uint32_t level;
  uint32_t leftChildIndex;
  uint32_t rightChildIndex;
  vec3 color;
  mat4x4 transformation;
} Node;

Node testNode(void);

#endif /* SRC_PLANT_UTILS_H_ */
