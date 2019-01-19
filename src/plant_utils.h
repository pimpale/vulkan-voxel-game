/* Copyright 2019 Govind Pimpale
 * plant_utils.h
 *
 *  Created on: Jan 16, 2019
 *      Author: gpi
 */

#ifndef SRC_PLANT_UTILS_H_
#define SRC_PLANT_UTILS_H_

#include "linmath.h"

struct Node {
  uint8_t type;
  float length;
  float area;
  vec3 color;
  mat4x4 transformation;
  struct Node *leftChild;
  struct Node *rightChild;
};

typedef struct Node Node;

/* TODO remove, for testing purposes only) */
Node testNode() {
  struct Node returnable;
  mat4x4_identity(returnable.transformation);
  returnable.return;
}

#endif /* SRC_PLANT_UTILS_H_ */
