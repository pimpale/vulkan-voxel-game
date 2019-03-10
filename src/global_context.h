/* Copyright 2019 Govind Pimpale
 * global_context.h
 *
 *  Created on: Aug 7, 2018
 *      Author: gpi
 */

#ifndef SRC_GLOBAL_CONTEXT_H_
#define SRC_GLOBAL_CONTEXT_H_

#include <stdint.h>

#include <vulkan/vulkan.h>

#include "errors.h"
#include "constants.h"

typedef struct {
  VkInstance instance;
  VkDebugUtilsMessengerEXT callback;

  uint32_t physicalDeviceCount;
  VkPhysicalDevice *pPhysicalDevices;
} CgeGlobalContext;

ErrVal new_CgeGlobalContext(CgeGlobalContext *globalContext);
void delete_CgeGlobalContext(CgeGlobalContext *globalContext);

ErrVal selectBestGraphicsDevices(CgeGlobalContext *globalContext,
                                 VkPhysicalDevice *pPhysicalDevices,
                                 uint32_t *physicalDeviceCount);
ErrVal selectBestComputeDevices(CgeGlobalContext *globalContext,
                                VkPhysicalDevice *pPhysicalDevices,
                                uint32_t *physicalDeviceCount);

#endif /* SRC_GLOBAL_CONTEXT_H_ */
