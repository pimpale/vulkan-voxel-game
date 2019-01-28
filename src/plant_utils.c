/* Copyright 2019 Govind Pimpale
 * plant_utils.c
 *
 *  Created on: Jan 20, 2019
 *      Author: gpi
 */

#include <stdint.h>
#include <stdlib.h>

#include <linmath.h>

#include "errors.h"
#include "plant_utils.h"
#include "utils.h"

void initNode(Node *pNode) {
  pNode->age = 0;
  pNode->leftChildIndex = UINT32_MAX;
  pNode->rightChildIndex = UINT32_MAX;
}

void initNodes(Node **ppNodes, uint32_t nodeCount) {
  *ppNodes = malloc(sizeof(Node) * nodeCount);
  for (uint32_t i = 0; i < nodeCount; i++) {
    initNode(&((*ppNodes)[i]));
  }
  return;
}

void updateNode(Node *pNode) { pNode->age++; }

void updateNodes(Node *pNodes, uint32_t nodeCount) {
  /* TODO parallelize or ship this off to the GPU */
  for (uint32_t i = 0; i < nodeCount; i++) {
    updateNode(&(pNodes[i]));
  }
}

void initVertexes(Vertex **ppVertexes, uint32_t vertexCount) {
  *ppVertexes = malloc(sizeof(Vertex) * vertexCount);
  if (!(*ppVertexes)) {
    LOG_ERROR(ERR_LEVEL_FATAL,
              "failed to initialize vertexes: could not allocate memory");
    PANIC();
  }
  return;
}

void recursiveGen(Vertex **ppVertexes, uint32_t vertexCount, const Node *pNodes,
                  const uint32_t nodeCount, uint32_t currentIndex,
                  vec3 parentPosition);

void recursiveGen(Vertex **ppVertexes, uint32_t vertexCount, const Node *pNodes,
                  const uint32_t nodeCount, uint32_t currentIndex,
                  vec3 parentPosition) {
  vec3 currentPosition;
  vec3_add(currentPosition, pNodes[currentIndex].displacement, parentPosition);
  if (pNodes[currentIndex].leftChildIndex != UINT32_MAX) {
    recursiveGen(ppVertexes, vertexCount, pNodes, nodeCount,
                 pNodes[currentIndex].leftChildIndex, currentPosition);
  }
  if (pNodes[currentIndex].rightChildIndex != UINT32_MAX) {
    recursiveGen(ppVertexes, vertexCount, pNodes, nodeCount,
                 pNodes[currentIndex].rightChildIndex, currentPosition);
  }
  /* TODO need to give real colors */
  vec3_dup((*ppVertexes)[currentIndex * 3 + 0].position, parentPosition);
  vec3_dup((*ppVertexes)[currentIndex * 3 + 1].position, currentPosition);
  vec3_dup((*ppVertexes)[currentIndex * 3 + 2].position, currentPosition);
  (*ppVertexes)[currentIndex * 3 + 2].position[2] += 0.1f;
  return;
}

void genVertexes(Vertex **ppVertexes, uint32_t *pVertexCount,
                 const Node *pNodes, const uint32_t nodeCount) {

  *pVertexCount = nodeCount * 3;
  (*ppVertexes) = realloc(*ppVertexes, *pVertexCount);
  if (*ppVertexes == NULL) {
    LOG_ERROR(ERR_LEVEL_FATAL,
              "failed to generate vertexes: could not allocate memory");
    PANIC();
  }
  for (uint32_t i = 0; i < nodeCount; i++) {
    if (pNodes[i].parentIndex == UINT32_MAX) {
      recursiveGen(ppVertexes, *pVertexCount, pNodes, nodeCount, i,
                   (vec3){0, i, 0});
    }
  }
  return;
}
