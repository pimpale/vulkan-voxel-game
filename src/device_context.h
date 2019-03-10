/* Copyright 2019 Govind Pimpale
 * device_context.h
 *
 *  Created on: Aug 7, 2018
 *      Author: gpi
 */

#ifndef SRC_DEVICE_CONTEXT_H_
#define SRC_DEVICE_CONTEXT_H_

#include <stdint.h>

#include <vulkan/vulkan.h>
#include <linmath.h>

#include "errors.h"

struct CgeDeviceContext {
  VkPhysicalDevice physicalDevice;
  VkDevice device;

  uint32_t graphicsIndex;
  uint32_t presentIndex;
  uint32_t computeIndex;
  
  VkQueue graphicsQueue;
  VkQueue presentQueue
  VkQueue computeQueue;
}

ErrVal new_CgeDeviceContext(CgeDeviceContext *deviceContext);
void delete_CgeDeviceContext(CgeDeviceContext * deviceContext);

#endif /* SRC_DEVICE_CONTEXT_H_ */
