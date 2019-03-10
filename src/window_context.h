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

#ifndef CGE_WINDOW_SWAPCHAIN_IMAGE_COUNT
#define CGE_WINDOW_SWAPCHAIN_IMAGE_COUNT 2
#endif /* CGE_WINDOW_SWAPCHAIN_IMAGE_COUNT */

struct CgeWindowContext {
  VkExtent2D windowDimension;
  VkSurfaceFormatKHR surfaceFormat;
  VkSwapChainKHR swapChain;
  uint32_t swapChainImageCount;
  VkImage *pSwapChainIamges;
  VkImageView *pSwapChainImageViews;
  VkFramebuffer *pSwapChainFramebuffers;

  VkImage depthImage;
  VkDeviceMemory depthImageMemory;
  VkImageView depthImageView;

  VkRenderPass renderPass;

  VkSemaphore *pImageAvailableSemaphores;
  VkSemaphore *pRenderFinishedSemaphores;
  VkFence *pInFlightFences;

  Transformation transform;
};

ErrVal new_CgeWindowContext(CgeWindowContext *windowContext);
void delete_CgeWindowContext(CgeWindowContext *windowContext);

#endif /* SRC_WINDOW_CONTEXT_H_ */
