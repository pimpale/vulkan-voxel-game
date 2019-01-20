/* Copyright 2019 Govind Pimpale
 * plant_utils.c
 *
 *  Created on: Jan 20, 2019
 *      Author: gpi
 */

#include <stdint.h>

#include "linmath.h"
#include "plant_utils.h"

/* TODO remove, for testing purposes only) */
Node testNode() {
  Node n = {0};
  mat4x4_identity(n.transformation);
  n.color[1] = 1.0f;
  n.leftChildIndex = 0;
  n.rightChildIndex = 0;
  return (n);
}
