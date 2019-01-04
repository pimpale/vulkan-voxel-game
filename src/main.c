#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "constants.h"
#include "errors.h"
#include "utils.h"
#include "vulkan_utils.h"

void transformView(bool *pModified, mat4x4 *pTransformation,
                   GLFWwindow *pWindow) {
  float dx = 0;
  float dy = 0;
  float dz = 0;
  if (glfwGetKey(pWindow, GLFW_KEY_A) == GLFW_PRESS) {
    dx += 0.01f;
    *pModified = true;
  }
  if (glfwGetKey(pWindow, GLFW_KEY_D) == GLFW_PRESS) {
    dx += -0.01f;
    *pModified = true;
  }
  if (glfwGetKey(pWindow, GLFW_KEY_W) == GLFW_PRESS) {
    dz += 0.01f;
    *pModified = true;
  }
  if (glfwGetKey(pWindow, GLFW_KEY_S) == GLFW_PRESS) {
    dz -= 0.01f;
    *pModified = true;
  }

  mat4x4_translate_in_place(*pTransformation, dx, dy, dz);

  float rx = 0.0f;
  float ry = 0.0f;
  float rz = 0.0f;

  if (glfwGetKey(pWindow, GLFW_KEY_UP) == GLFW_PRESS) {
    ry += 0.01f;
    *pModified = true;
  }
  if (glfwGetKey(pWindow, GLFW_KEY_DOWN) == GLFW_PRESS) {
    ry += -0.01f;
    *pModified = true;
  }
  if (glfwGetKey(pWindow, GLFW_KEY_LEFT) == GLFW_PRESS) {
    rx += 0.01f;
    *pModified = true;
  }
  if (glfwGetKey(pWindow, GLFW_KEY_RIGHT) == GLFW_PRESS) {
    rx -= 0.01f;
    *pModified = true;
  }
  mat4x4_rotate_X(*pTransformation, *pTransformation, rx);
  mat4x4_rotate_Y(*pTransformation, *pTransformation, ry);
  mat4x4_rotate_Z(*pTransformation, *pTransformation, rz);
}

int main(void) {
  glfwInit();

  /* Extensions, Layers, and Device Extensions declared
   */
  uint32_t extensionCount;
  char **ppExtensionNames;
  new_RequiredInstanceExtensions(&extensionCount, &ppExtensionNames);

  uint32_t layerCount;
  char **ppLayerNames;
  new_ValidationLayers(&layerCount, &ppLayerNames);

  uint32_t graphicsDeviceExtensionCount;
  char **ppDeviceExtensionNames;
  new_RequiredDeviceExtensions(&graphicsDeviceExtensionCount,
                               &ppDeviceExtensionNames);

  /* Create instance */
  VkInstance instance;
  new_Instance(&instance, extensionCount, (const char *const *)ppExtensionNames,
               layerCount, (const char *const *)ppLayerNames);
  VkDebugUtilsMessengerEXT callback;
  new_DebugCallback(&callback, instance);

  /* get physical graphicsDevice */
  VkPhysicalDevice physicalDevice;
  getPhysicalDevice(&physicalDevice, instance);

  /* Create window and surface */
  GLFWwindow *pWindow;
  new_GLFWwindow(&pWindow);
  VkSurfaceKHR surface;
  new_SurfaceFromGLFW(&surface, pWindow, instance);

  /* find queues on graphics graphicsDevice */
  uint32_t graphicsIndex;
  uint32_t computeIndex;
  uint32_t presentIndex;
  {
    uint32_t ret1 = getDeviceQueueIndex(&graphicsIndex, physicalDevice,
                                        VK_QUEUE_GRAPHICS_BIT);
    uint32_t ret2 = getDeviceQueueIndex(&computeIndex, physicalDevice,
                                        VK_QUEUE_COMPUTE_BIT);
    uint32_t ret3 =
        getPresentQueueIndex(&presentIndex, physicalDevice, surface);
    /* Panic if indices are unavailable */
    if (ret1 != VK_SUCCESS || ret2 != VK_SUCCESS || ret3 != VK_SUCCESS) {
      errLog(FATAL, "unable to acquire indices\n");
      panic();
    }
  }
	/* Set up compute */

	VkDevice computeDevice;

	new_Device(&computeDevice, physicalDevice, computeIndex, 0, NULL, layerCount, 

             (const char *const *)ppLayerNames);
  VkPipelineLayout nodeUpdatePipelineLayout;
  VkPipelineLayout nodeTopologyPipelineLayout;
  VkPipelineLayout vertexGenerationPipelineLayout;

  new_NodeUpdateComputePipelineLayout(&nodeUpdatePipelineLayout,
                                      computeDevice);
  new_NodeTopologyComputePipelineLayout(&nodeTopologyPipelineLayout,
                                        computeDevice);
  new_VertexGenerationComputePipelineLayout(&vertexGenerationPipelineLayout,
                                            computeDevice);

  /* Shader modules */
  VkShaderModule nodeUpdateShaderModule;
  VkShaderModule nodeTopologyShaderModule;
  VkShaderModule vertexGenerationShaderModule;

  /* Load from file */

  VkPipeline nodeUpdatePipeline;
  VkPipeline nodeTopologyPipeline;
  VkPipeline vertexGenerationPipeline;

  new_ComputePipelines(&nodeUpdatePipeline, &nodeTopologyPipeline,
                       &vertexGenerationPipeline, nodeUpdatePipelineLayout,
                       nodeTopologyPipelineLayout,
                       vertexGenerationPipelineLayout, 
											 nodeUpdateShaderModule, nodeTopologyShaderModule,
											 vertexGenerationShaderModule, computeDevice);

  /* Set extent (for now just window width and height) */
  VkExtent2D swapChainExtent;
  getWindowExtent(&swapChainExtent, pWindow);

  /*create graphicsDevice */
  VkDevice graphicsDevice;
  new_Device(&graphicsDevice, physicalDevice, graphicsIndex,
             graphicsDeviceExtensionCount,
             (const char *const *)ppDeviceExtensionNames, layerCount,
             (const char *const *)ppLayerNames);

  VkQueue graphicsQueue;
  getQueue(&graphicsQueue, graphicsDevice, graphicsIndex);
  VkQueue computeQueue;
  getQueue(&computeQueue, graphicsDevice, computeIndex);
  VkQueue presentQueue;
  getQueue(&presentQueue, graphicsDevice, presentIndex);

  VkCommandPool commandPool;
  new_CommandPool(&commandPool, graphicsDevice, graphicsIndex);

  /* get preferred format of screen*/
  VkSurfaceFormatKHR surfaceFormat;
  getPreferredSurfaceFormat(&surfaceFormat, physicalDevice, surface);

  /*Create swap chain */
  VkSwapchainKHR swapChain;
  uint32_t swapChainImageCount = 0;
  new_SwapChain(&swapChain, &swapChainImageCount, VK_NULL_HANDLE, surfaceFormat,
                physicalDevice, graphicsDevice, surface, swapChainExtent,
                graphicsIndex, presentIndex);

  VkImage *pSwapChainImages = NULL;
  VkImageView *pSwapChainImageViews = NULL;

  new_SwapChainImages(&pSwapChainImages, &swapChainImageCount, graphicsDevice,
                      swapChain);
  new_SwapChainImageViews(&pSwapChainImageViews, graphicsDevice,
                          surfaceFormat.format, swapChainImageCount,
                          pSwapChainImages);

  VkImage depthImage;
  VkDeviceMemory depthImageMemory;
  new_DepthImage(&depthImage, &depthImageMemory, swapChainExtent,
                 physicalDevice, graphicsDevice, commandPool, graphicsQueue);
  VkImageView depthImageView;
  new_DepthImageView(&depthImageView, graphicsDevice, depthImage);

  VkShaderModule fragShaderModule;
  new_ShaderModuleFromFile(&fragShaderModule, graphicsDevice,
                           "assets/shaders/shader.frag.spv");
  VkShaderModule vertShaderModule;
  new_ShaderModuleFromFile(&vertShaderModule, graphicsDevice,
                           "assets/shaders/shader.vert.spv");

  /* Create graphics pipeline */
  VkRenderPass renderPass;
  new_VertexDisplayRenderPass(&renderPass, graphicsDevice,
                              surfaceFormat.format);

  VkPipelineLayout vertexDisplayPipelineLayout;
  new_VertexDisplayPipelineLayout(&vertexDisplayPipelineLayout, graphicsDevice);

  VkPipeline vertexDisplayPipeline;
  new_VertexDisplayPipeline(&vertexDisplayPipeline, graphicsDevice,
                            vertShaderModule, fragShaderModule, swapChainExtent,
                            renderPass, vertexDisplayPipelineLayout);

  VkFramebuffer *pSwapChainFramebuffers;
  new_SwapChainFramebuffers(&pSwapChainFramebuffers, graphicsDevice, renderPass,
                            swapChainExtent, swapChainImageCount,
                            depthImageView, pSwapChainImageViews);

#define VERTEXNUM 12

  struct Vertex vertices[VERTEXNUM] = {{{-1, -1, 0}, {1, 0, 0}},
                                       {{1, -1, 0}, {1, 1, 1}},
                                       {{1, 1, 0}, {0, 1, 0}},
                                       {{1, 1, 0}, {0, 1, 0}},
                                       {{-1, 1, 0}, {1, 1, 1}},
                                       {{-1, -1, 0}, {1, 0, 0}},

                                       /* Square 2 */
                                       {{-1, -1, 1}, {0, 1, 0}},
                                       {{1, -1, 1}, {1, 1, 1}},
                                       {{1, 1, 1}, {0, 1, 0}},
                                       {{1, 1, 1}, {0, 1, 0}},
                                       {{-1, 1, 1}, {1, 1, 1}},
                                       {{-1, -1, 1}, {0, 1, 0}}

  };

  /* The final result to be pushed */
  mat4x4 cameraViewProduct;
  mat4x4 cameraViewModel;
  mat4x4 cameraViewView;
  mat4x4 cameraViewProjection;

  mat4x4_identity(cameraViewModel);
  mat4x4_identity(cameraViewView);
  mat4x4_identity(cameraViewProjection);

  mat4x4_look_at(cameraViewView, (vec3){2.0f, 2.0f, 2.0f},
                 (vec3){0.0f, 0.0f, 0.0f}, (vec3){0.0f, 0.0f, 1.0f});
  mat4x4_perspective(cameraViewProjection, 1,
                     ((float)swapChainExtent.width) / swapChainExtent.height,
                     0.1f, 10.0f); /* The 1 is in radians */

  mat4x4_mul(cameraViewProduct, cameraViewProjection, cameraViewView);
  mat4x4_mul(cameraViewProduct, cameraViewProduct, cameraViewModel);

  VkBuffer vertexBuffer;
  VkDeviceMemory vertexBufferMemory;
  new_VertexBuffer(&vertexBuffer, &vertexBufferMemory, vertices, VERTEXNUM,
                   graphicsDevice, physicalDevice, commandPool, graphicsQueue);

  VkCommandBuffer *pVertexDisplayCommandBuffers;
  new_VertexDisplayCommandBuffers(
      &pVertexDisplayCommandBuffers, vertexBuffer, VERTEXNUM, graphicsDevice,
      renderPass, vertexDisplayPipelineLayout, vertexDisplayPipeline,
      commandPool, swapChainExtent, swapChainImageCount,
      (const VkFramebuffer *)pSwapChainFramebuffers, cameraViewProduct);

  VkSemaphore *pImageAvailableSemaphores;
  VkSemaphore *pRenderFinishedSemaphores;
  VkFence *pInFlightFences;
  new_Semaphores(&pImageAvailableSemaphores, swapChainImageCount,
                 graphicsDevice);
  new_Semaphores(&pRenderFinishedSemaphores, swapChainImageCount,
                 graphicsDevice);
  new_Fences(&pInFlightFences, swapChainImageCount, graphicsDevice);

  uint32_t currentFrame = 0;
  /*wait till close*/
  while (!glfwWindowShouldClose(pWindow)) {
    glfwPollEvents();

    bool viewModified = false;
    transformView(&viewModified, &cameraViewProduct, pWindow);
    if (viewModified) {
      vkQueueWaitIdle(graphicsQueue);
      delete_CommandBuffers(&pVertexDisplayCommandBuffers, swapChainImageCount,
                            commandPool, graphicsDevice);
      new_VertexDisplayCommandBuffers(
          &pVertexDisplayCommandBuffers, vertexBuffer, VERTEXNUM,
          graphicsDevice, renderPass, vertexDisplayPipelineLayout,
          vertexDisplayPipeline, commandPool, swapChainExtent,
          swapChainImageCount, pSwapChainFramebuffers, cameraViewProduct);
    }

    ErrVal result =
        drawFrame(&currentFrame, 2, graphicsDevice, swapChain,
                  pVertexDisplayCommandBuffers, pInFlightFences,
                  pImageAvailableSemaphores, pRenderFinishedSemaphores,
                  graphicsQueue, presentQueue);

    if (result == ERR_OUTOFDATE) {
      vkQueueWaitIdle(graphicsQueue);
      delete_Fences(&pInFlightFences, swapChainImageCount, graphicsDevice);
      delete_Semaphores(&pRenderFinishedSemaphores, swapChainImageCount,
                        graphicsDevice);
      delete_Semaphores(&pImageAvailableSemaphores, swapChainImageCount,
                        graphicsDevice);
      delete_CommandBuffers(&pVertexDisplayCommandBuffers, swapChainImageCount,
                            commandPool, graphicsDevice);
      delete_SwapChainFramebuffers(&pSwapChainFramebuffers, swapChainImageCount,
                                   graphicsDevice);
      delete_Pipeline(&vertexDisplayPipeline, graphicsDevice);
      delete_PipelineLayout(&vertexDisplayPipelineLayout, graphicsDevice);
      delete_Image(&depthImage, graphicsDevice);
      delete_DeviceMemory(&depthImageMemory, graphicsDevice);
      delete_ImageView(&depthImageView, graphicsDevice);
      delete_RenderPass(&renderPass, graphicsDevice);
      delete_SwapChainImageViews(&pSwapChainImageViews, swapChainImageCount,
                                 graphicsDevice);
      delete_SwapChainImages(&pSwapChainImages);
      delete_SwapChain(&swapChain, graphicsDevice);

      /* Set swapchain to new window size */
      getWindowExtent(&swapChainExtent, pWindow);

      /*Create swap chain */
      new_SwapChain(&swapChain, &swapChainImageCount, VK_NULL_HANDLE,
                    surfaceFormat, physicalDevice, graphicsDevice, surface,
                    swapChainExtent, graphicsIndex, presentIndex);
      new_SwapChainImages(&pSwapChainImages, &swapChainImageCount,
                          graphicsDevice, swapChain);
      new_SwapChainImageViews(&pSwapChainImageViews, graphicsDevice,
                              surfaceFormat.format, swapChainImageCount,
                              pSwapChainImages);

      new_DepthImage(&depthImage, &depthImageMemory, swapChainExtent,
                     physicalDevice, graphicsDevice, commandPool,
                     graphicsQueue);
      new_DepthImageView(&depthImageView, graphicsDevice, depthImage);
      /* Create graphics pipeline */
      new_VertexDisplayRenderPass(&renderPass, graphicsDevice,
                                  surfaceFormat.format);
      new_VertexDisplayPipelineLayout(&vertexDisplayPipelineLayout,
                                      graphicsDevice);
      new_VertexDisplayPipeline(&vertexDisplayPipeline, graphicsDevice,
                                vertShaderModule, fragShaderModule,
                                swapChainExtent, renderPass,
                                vertexDisplayPipelineLayout);
      new_SwapChainFramebuffers(
          &pSwapChainFramebuffers, graphicsDevice, renderPass, swapChainExtent,
          swapChainImageCount, depthImageView, pSwapChainImageViews);

      new_VertexDisplayCommandBuffers(
          &pVertexDisplayCommandBuffers, vertexBuffer, VERTEXNUM,
          graphicsDevice, renderPass, vertexDisplayPipelineLayout,
          vertexDisplayPipeline, commandPool, swapChainExtent,
          swapChainImageCount, pSwapChainFramebuffers, cameraViewProduct);
      new_Semaphores(&pImageAvailableSemaphores, swapChainImageCount,
                     graphicsDevice);
      new_Semaphores(&pRenderFinishedSemaphores, swapChainImageCount,
                     graphicsDevice);
      new_Fences(&pInFlightFences, swapChainImageCount, graphicsDevice);
    }
  }

  /*cleanup*/
  vkDeviceWaitIdle(graphicsDevice);
  delete_ShaderModule(&fragShaderModule, graphicsDevice);
  delete_ShaderModule(&vertShaderModule, graphicsDevice);
  delete_Fences(&pInFlightFences, swapChainImageCount, graphicsDevice);
  delete_Semaphores(&pRenderFinishedSemaphores, swapChainImageCount,
                    graphicsDevice);
  delete_Semaphores(&pImageAvailableSemaphores, swapChainImageCount,
                    graphicsDevice);
  delete_CommandBuffers(&pVertexDisplayCommandBuffers, swapChainImageCount,
                        commandPool, graphicsDevice);
  delete_CommandPool(&commandPool, graphicsDevice);
  delete_SwapChainFramebuffers(&pSwapChainFramebuffers, swapChainImageCount,
                               graphicsDevice);
  delete_Pipeline(&vertexDisplayPipeline, graphicsDevice);
  delete_PipelineLayout(&vertexDisplayPipelineLayout, graphicsDevice);
  delete_Buffer(&vertexBuffer, graphicsDevice);
  delete_DeviceMemory(&vertexBufferMemory, graphicsDevice);
  delete_RenderPass(&renderPass, graphicsDevice);
  delete_Image(&depthImage, graphicsDevice);
  delete_DeviceMemory(&depthImageMemory, graphicsDevice);
  delete_ImageView(&depthImageView, graphicsDevice);
  delete_SwapChainImageViews(&pSwapChainImageViews, swapChainImageCount,
                             graphicsDevice);
  delete_SwapChainImages(&pSwapChainImages);
  delete_SwapChain(&swapChain, graphicsDevice);
  delete_Device(&graphicsDevice);
  delete_Surface(&surface, instance);
  delete_DebugCallback(&callback, instance);
  delete_Instance(&instance);
  delete_RequiredDeviceExtensions(&graphicsDeviceExtensionCount,
                                  &ppDeviceExtensionNames);
  delete_ValidationLayers(&layerCount, &ppLayerNames);
  delete_RequiredInstanceExtensions(&extensionCount, &ppExtensionNames);
  glfwTerminate();
  return (EXIT_SUCCESS);
}
