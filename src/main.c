#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <linmath.h>

#include "constants.h"
#include "errors.h"
#include "plant_utils.h"
#include "utils.h"
#include "vulkan_utils.h"

uint32_t extensionCount;
char **ppExtensionNames;

uint32_t layerCount;
char **ppLayerNames;

uint32_t graphicsDeviceExtensionCount;
char **ppDeviceExtensionNames;

VkInstance instance;
VkDebugUtilsMessengerEXT callback;

VkPhysicalDevice physicalDevice;
uint32_t graphicsIndex;
uint32_t computeIndex;
uint32_t presentIndex;

GLFWwindow *pWindow;
VkSurfaceKHR surface;

/* Compute */
VkDevice computeDevice;
VkBuffer nodeBuffer;
VkDeviceSize nodeBufferSize;
VkDeviceMemory nodeBufferDeviceMemory;
VkDescriptorSetLayout nodeBufferDescriptorSetLayout;
VkPipelineLayout nodeUpdatePipelineLayout;
VkShaderModule nodeUpdateShaderModule;
VkPipeline nodeUpdatePipeline;
VkCommandPool computeCommandPool;
VkDescriptorPool computeDescriptorPool;
VkDescriptorSet computeBufferDescriptorSet;
VkCommandBuffer computeCommandBuffer;

VkExtent2D swapChainExtent;
VkDevice graphicsDevice;
VkQueue graphicsQueue;
VkQueue presentQueue;
VkCommandPool commandPool;
VkSurfaceFormatKHR surfaceFormat;

VkSwapchainKHR swapChain;
uint32_t swapChainImageCount = 0;
VkImage *pSwapChainImages = NULL;
VkImageView *pSwapChainImageViews = NULL;

VkImage depthImage;
VkDeviceMemory depthImageMemory;
VkImageView depthImageView;

VkBuffer vertexBuffer;
VkDeviceMemory vertexBufferMemory;

VkShaderModule fragShaderModule;
VkShaderModule vertShaderModule;

VkRenderPass renderPass;

VkPipelineLayout vertexDisplayPipelineLayout;
VkPipeline vertexDisplayPipeline;

VkFramebuffer *pSwapChainFramebuffers;

Transformation transform;
mat4x4 mvp;

VkCommandBuffer *pVertexDisplayCommandBuffers;

VkSemaphore *pImageAvailableSemaphores;
VkSemaphore *pRenderFinishedSemaphores;
VkFence *pInFlightFences;

int main(void) {
  glfwInit();

  /* Extensions, Layers, and Device Extensions declared */
  new_RequiredInstanceExtensions(&extensionCount, &ppExtensionNames);
  new_ValidationLayers(&layerCount, &ppLayerNames);
  new_RequiredDeviceExtensions(&graphicsDeviceExtensionCount,
                               &ppDeviceExtensionNames);

  /* Create instance */
  new_Instance(&instance, extensionCount, (const char *const *)ppExtensionNames,
               layerCount, (const char *const *)ppLayerNames);
  new_DebugCallback(&callback, instance);

  /* get physical graphicsDevice */
  getPhysicalDevice(&physicalDevice, instance);

  /* Create window and surface */
  new_GLFWwindow(&pWindow);
  new_SurfaceFromGLFW(&surface, pWindow, instance);

  /* find queues on physical device*/
  {
    uint32_t ret1 = getDeviceQueueIndex(&graphicsIndex, physicalDevice,
                                        VK_QUEUE_GRAPHICS_BIT);
    uint32_t ret2 = getDeviceQueueIndex(&computeIndex, physicalDevice,
                                        VK_QUEUE_COMPUTE_BIT);
    uint32_t ret3 =
        getPresentQueueIndex(&presentIndex, physicalDevice, surface);
    /* Panic if indices are unavailable */
    if (ret1 != VK_SUCCESS || ret2 != VK_SUCCESS || ret3 != VK_SUCCESS) {
      LOG_ERROR(ERR_LEVEL_FATAL, "unable to acquire indices");
      PANIC();
    }
  }
  /* Set up compute */
  /* Create device*/
  new_Device(&computeDevice, physicalDevice, computeIndex, 0, NULL, layerCount,
             (const char *const *)ppLayerNames);

  /* Allocate memory for buffers */
  /* One node for now */
  new_Buffer_DeviceMemory(&nodeBuffer, &nodeBufferDeviceMemory, nodeBufferSize,
                          physicalDevice, computeDevice,
                          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  nodeBufferSize = sizeof(Node) * 1;
  /* Initialize node buffer memory */
  /* copyToDeviceMemory(&nodeBufferDeviceMemory, nodeBufferSize, pNodeData,
                     computeDevice); */

  /* Create Descriptor set layout for a node buffer */
  new_ComputeStorageDescriptorSetLayout(&nodeBufferDescriptorSetLayout,
                                        computeDevice);
  /* Create Descriptor pool */
  new_DescriptorPool(&computeDescriptorPool, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                     3, computeDevice);
  /* Create Descriptor sets */
  new_ComputeBufferDescriptorSet(&computeBufferDescriptorSet, nodeBuffer,
                                 nodeBufferSize, nodeBufferDescriptorSetLayout,
                                 computeDescriptorPool, computeDevice);
  /* Create PipelineLayout */
  new_NodeUpdateComputePipelineLayout(
      &nodeUpdatePipelineLayout, nodeBufferDescriptorSetLayout, computeDevice);
  /*Load Shader modules from file */
  new_ShaderModuleFromFile(&nodeUpdateShaderModule, computeDevice,
                           "assets/shaders/nodeupdate.comp.spv");
  new_ComputePipeline(&nodeUpdatePipeline, nodeUpdatePipelineLayout,
                      nodeUpdateShaderModule, computeDevice);

  /* Create command pool */
  new_CommandPool(&computeCommandPool, computeDevice, computeIndex);

  /* Create command buffer */
  new_begin_OneTimeSubmitCommandBuffer(&computeCommandBuffer, computeDevice,
                                       computeCommandPool);

  /* Set extent (for now just window width and height) */
  getWindowExtent(&swapChainExtent, pWindow);

  /*create graphicsDevice */
  new_Device(&graphicsDevice, physicalDevice, graphicsIndex,
             graphicsDeviceExtensionCount,
             (const char *const *)ppDeviceExtensionNames, layerCount,
             (const char *const *)ppLayerNames);

  getQueue(&graphicsQueue, graphicsDevice, graphicsIndex);
  getQueue(&presentQueue, graphicsDevice, presentIndex);

  new_CommandPool(&commandPool, graphicsDevice, graphicsIndex);

  /* get preferred format of screen*/
  getPreferredSurfaceFormat(&surfaceFormat, physicalDevice, surface);

  /*Create swap chain */
  new_SwapChain(&swapChain, &swapChainImageCount, VK_NULL_HANDLE, surfaceFormat,
                physicalDevice, graphicsDevice, surface, swapChainExtent,
                graphicsIndex, presentIndex);

  new_SwapChainImages(&pSwapChainImages, &swapChainImageCount, graphicsDevice,
                      swapChain);
  new_SwapChainImageViews(&pSwapChainImageViews, graphicsDevice,
                          surfaceFormat.format, swapChainImageCount,
                          pSwapChainImages);

  new_DepthImage(&depthImage, &depthImageMemory, swapChainExtent,
                 physicalDevice, graphicsDevice, commandPool, graphicsQueue);
  new_DepthImageView(&depthImageView, graphicsDevice, depthImage);

  new_ShaderModuleFromFile(&fragShaderModule, graphicsDevice,
                           "assets/shaders/vertexdisplay.frag.spv");
  new_ShaderModuleFromFile(&vertShaderModule, graphicsDevice,
                           "assets/shaders/vertexdisplay.vert.spv");

  /* Create graphics pipeline */
  new_VertexDisplayRenderPass(&renderPass, graphicsDevice,
                              surfaceFormat.format);

  new_VertexDisplayPipelineLayout(&vertexDisplayPipelineLayout, graphicsDevice);

  new_VertexDisplayPipeline(&vertexDisplayPipeline, graphicsDevice,
                            vertShaderModule, fragShaderModule, swapChainExtent,
                            renderPass, vertexDisplayPipelineLayout);

  new_SwapChainFramebuffers(&pSwapChainFramebuffers, graphicsDevice, renderPass,
                            swapChainExtent, swapChainImageCount,
                            depthImageView, pSwapChainImageViews);

  new_VertexBuffer(&vertexBuffer, &vertexBufferMemory, vertexList, VERTEXNUM,
                   graphicsDevice, physicalDevice, commandPool, graphicsQueue);

  /* Set up transformation */
  initTransformation(&transform);
  matFromTransformation(&mvp, transform, swapChainExtent.width,
                        swapChainExtent.height);

  new_VertexDisplayCommandBuffers(
      &pVertexDisplayCommandBuffers, vertexBuffer, VERTEXNUM, graphicsDevice,
      renderPass, vertexDisplayPipelineLayout, vertexDisplayPipeline,
      commandPool, swapChainExtent, swapChainImageCount,
      (const VkFramebuffer *)pSwapChainFramebuffers, cameraViewProduct);

  new_Semaphores(&pImageAvailableSemaphores, swapChainImageCount,
                 graphicsDevice);
  new_Semaphores(&pRenderFinishedSemaphores, swapChainImageCount,
                 graphicsDevice);
  new_Fences(&pInFlightFences, swapChainImageCount, graphicsDevice);

  uint32_t currentFrame = 0;
  /*wait till close*/
  while (!glfwWindowShouldClose(pWindow)) {
    glfwPollEvents();

    vkQueueWaitIdle(graphicsQueue);
    delete_CommandBuffers(&pVertexDisplayCommandBuffers, swapChainImageCount,
                          commandPool, graphicsDevice);
    new_VertexDisplayCommandBuffers(
        &pVertexDisplayCommandBuffers, vertexBuffer, VERTEXNUM, graphicsDevice,
        renderPass, vertexDisplayPipelineLayout, vertexDisplayPipeline,
        commandPool, swapChainExtent, swapChainImageCount,
        pSwapChainFramebuffers, cameraViewProduct);

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
