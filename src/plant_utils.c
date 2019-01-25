/* Copyright 2019 Govind Pimpale
 * plant_utils.c
 *
 *  Created on: Jan 20, 2019
 *      Author: gpi
 */

#include <stdint.h>

#include <linmath.h>
#include "plant_utils.h"

void initNode(Node *pNode) {
  pNode->color[0] = 0.0f;
  pNode->color[1] = 1.0f;
  pNode->color[2] = 0.0f;
  pNode->leftChildIndex = UINT32_MAX;
  pNode->rightChildIndex = UINT32_MAX;
  mat4x4_identity(pNode->transformation);
}

void updateNode(Node *pNode) {

}

void updateNodes(Node *pNodes, uint32_t nodeCount) {}
