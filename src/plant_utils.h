/* Copyright 2019 Govind Pimpale
 * plant_utils.h
 *
 *  Created on: Jan 16, 2019
 *      Author: gpi
 */

#ifndef SRC_PLANT_UTILS_H_
#define SRC_PLANT_UTILS_H_

#include <linmath.h>
#include <stdint.h>
#include <stdio.h>

#include "utils.h"

typedef enum {
  NODE_CMD_NONE = 0,
  NODE_CMD_TYPE_RECURSIVE = 1,
  NODE_CMD_DELETE_LEFT_NODE = 2,
  NODE_CMD_DELETE_RIGHT_NODE = 4,
  NODE_CMD_DELETE_SELF_NODE = 4,
  /* TODO need to sliders, for now we'll be processing this in C */
} NodeCommand;

typedef struct {
  /*The total number of nodes in the buffer*/
  uint32_t nodeCount;
  /* 1 when node has been added or deleted, 0 otherwise */
  uint32_t needTopologyUpdate;
} NodeUpdateGlobalValues;

typedef struct {
  /*
   * Node topology data
   * UINT32_MAX is a sentinel value signifying a null connection
   */
  uint32_t leftChildIndex;
  uint32_t rightChildIndex;
  /*
   * Data about node status
   */
  uint32_t age;         /* Age in ticks */
  uint32_t area;        /* Area in mm^2 */
  uint32_t temperature; /* Temperature in Kelvin */

  /*
   * A command signalling an action to be completed during the topology update
   * phase
   */
  NodeCommand command;

  /*
   * Data about node position and color to be used during vertex generation
   */
  vec3 color;
  mat4x4 transformation; /* Transformation from parent element. */

  /*
   * Volatile data that may be changed
   */
} NodeReal;

typedef struct {
  uint32_t leftChildIndex;
  uint32_t rightChildIndex;
  uint32_t parentIndex;
  uint32_t age;
  float width;
  vec3 displacement;
} Node;

void initNode(Node *pNode);
void initNodes(Node **ppNodes, uint32_t nodeCount);
void updateNode(Node *pNode);
void updateNodes(Node *pNodes, uint32_t nodeCount);
void initVertexes(Vertex **ppVertexes, uint32_t vertexCount);
void genVertexes(Vertex **ppVertexes, uint32_t *pVertexCount,
                 const Node *pNodes, const uint32_t nodeCount);
#endif /* SRC_PLANT_UTILS_H_ */
