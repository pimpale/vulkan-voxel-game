/* Copyright 2019 Govind Pimpale
 * window_context.h
 *
 *  Created on: Aug 7, 2018
 *      Author: gpi
 */

#ifndef SRC_WINDOW_CONTEXT_H_
#define SRC_WINDOW_CONTEXT_H_

#include <stdint.h>
#include <vulkan/vulkan.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <linmath.h>

#include "errors.h"

struct CgeWindowContext {
  VkPhysicalWindow physicalWindow;
  VkWindow window;

  uint32_t graphicsIndex;
  uint32_t presentIndex;
  uint32_t computeIndex;
  
  VkQueue graphicsQueue;
  VkQueue presentQueue
  VkQueue computeQueue;
}

ErrVal new_CgeWindowContext(CgeWindowContext *windowContext);
void delete_CgeWindowContext(CgeWindowContext *windowContext);



#endif /* SRC_WINDOW_CONTEXT_H_ */
