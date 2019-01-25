/* Copyright 2019 Govind Pimpale
 * plant_utils.c
 *
 *  Created on: Jan 20, 2019
 *      Author: gpi
 */

#include <stdint.h>

#include "plant_utils.h"
#include <linmath.h>

#include "utils.h"

void initNode(Node *pNode) {
  pNode->color[0] = 0.0f;
  pNode->color[1] = 1.0f;
  pNode->color[2] = 0.0f;
  pNode->leftChildIndex = UINT32_MAX;
  pNode->rightChildIndex = UINT32_MAX;
  mat4x4_identity(pNode->transformation);
}

void updateNode(Node *pNode) {
  pNode->age++;
  pNode->area += 1;
}

void updateNodes(Node *pNodes, uint32_t nodeCount) {
  /* TODO parallelize or ship this off to the GPU */
  for (uint32_t i = 0; i < nodeCount; i++) {
    updateNode(&(pNodes[i]));
  }
}

void genVertexes(Vertex **ppVertexes, uint32_t vertexCount, const Node *pNodes,
                 const uint32_t nodeCount) {
  for (uint32_t i = 0; i < nodeCount; i++) {
    /* If it is a root node */
    if (pNodes[i].parentIndex == UINT32_MAX) {
    }
  }
}
