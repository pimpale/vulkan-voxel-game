/* Copyright 2019 Govind Pimpale
 * plant_utils.h
 *
 *  Created on: Jan 16, 2019
 *      Author: gpi
 */

#ifndef SRC_PLANT_UTILS_H_
#define SRC_PLANT_UTILS_H_

#include <stdint.h>

#include <linmath.h>

typedef enum {
  NODE_CMD_NONE = 0,
  NODE_CMD_DELETE_SELF = 1,
  NODE_CMD_DELETE_LEFT_NODE = 2,
  NODE_CMD_DELETE_RIGHT_NODE = 4,
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
  uint32_t parentIndex;
  uint32_t leftChildIndex;
  uint32_t rightChildIndex;
  /*
   * Data about node status
   */
  uint32_t age; /* Age in ticks */
  float length;

  NodeCommand command;
  vec3 color;
  mat4x4 transformation;
} Node;

#endif /* SRC_PLANT_UTILS_H_ */
