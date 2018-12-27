#include <errno.h>
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

int main(void) {
  glfwInit();

  /* Extensions, Layers, and Device Extensions declared */
  uint32_t extensionCount;
  char **ppExtensionNames;
  new_RequiredInstanceExtensions(&extensionCount, &ppExtensionNames);

  uint32_t layerCount;
  char **ppLayerNames;
  new_ValidationLayers(&layerCount, &ppLayerNames);

  uint32_t deviceExtensionCount;
  char **ppDeviceExtensionNames;
  new_RequiredDeviceExtensions(&deviceExtensionCount, &ppDeviceExtensionNames);

  /* Create instance */
  VkInstance instance;
  new_Instance(&instance, extensionCount, (const char *const *)ppExtensionNames,
               layerCount, (const char *const *)ppLayerNames);
  VkDebugUtilsMessengerEXT callback;
  new_DebugCallback(&callback, instance);

  /* get physical device */
  VkPhysicalDevice physicalDevice;
  getPhysicalDevice(&physicalDevice, instance);

  /* Create window and surface */
  GLFWwindow *pWindow;
  new_GLFWwindow(&pWindow);
  VkSurfaceKHR surface;
  new_SurfaceFromGLFW(&surface, pWindow, instance);

  /* find queues on graphics device */
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

  /* Set extent (for now just window width and height) */
  VkExtent2D swapChainExtent;
  getWindowExtent(&swapChainExtent, pWindow);

  /*create device */
  VkDevice device;
  new_Device(&device, physicalDevice, graphicsIndex, deviceExtensionCount,
             (const char *const *)ppDeviceExtensionNames, layerCount,
             (const char *const *)ppLayerNames);

  VkQueue graphicsQueue;
  getQueue(&graphicsQueue, device, graphicsIndex);
  VkQueue computeQueue;
  getQueue(&computeQueue, device, computeIndex);
  VkQueue presentQueue;
  getQueue(&presentQueue, device, presentIndex);

  /* get preferred format of screen*/
  VkSurfaceFormatKHR surfaceFormat;
  getPreferredSurfaceFormat(&surfaceFormat, physicalDevice, surface);

  /*Create swap chain */
  VkSwapchainKHR swapChain;
  uint32_t swapChainImageCount = 0;
  new_SwapChain(&swapChain, &swapChainImageCount, VK_NULL_HANDLE, surfaceFormat,
                physicalDevice, device, surface, swapChainExtent, graphicsIndex,
                presentIndex);

  VkImage *pSwapChainImages = NULL;
  VkImageView *pSwapChainImageViews = NULL;

  new_SwapChainImages(&pSwapChainImages, &swapChainImageCount, device,
                      swapChain);
  new_SwapChainImageViews(&pSwapChainImageViews, device, surfaceFormat.format,
                          swapChainImageCount, pSwapChainImages);

  VkShaderModule fragShaderModule;
  new_ShaderModuleFromFile(&fragShaderModule, device,
                           "assets/shaders/shader.frag.spv");
  VkShaderModule vertShaderModule;
  new_ShaderModuleFromFile(&vertShaderModule, device,
                           "assets/shaders/shader.vert.spv");

  /* Create graphics pipeline */
  VkRenderPass renderPass;
  new_RenderPass(&renderPass, device, surfaceFormat.format);

  VkDescriptorSetLayout modelViewProjectionDescriptorSetLayout;
  new_ModelViewProjectionDescriptorSetLayout(
      &modelViewProjectionDescriptorSetLayout, device);

  VkPipelineLayout vertexDisplayPipelineLayout;
  new_VertexDisplayPipelineLayout(&vertexDisplayPipelineLayout,
                                  modelViewProjectionDescriptorSetLayout,
                                  device);

  VkPipeline vertexDisplayPipeline;
  new_VertexDisplayPipeline(&vertexDisplayPipeline, device, vertShaderModule,
                            fragShaderModule, swapChainExtent, renderPass,
                            vertexDisplayPipelineLayout);

  VkFramebuffer *pSwapChainFramebuffers;
  new_SwapChainFramebuffers(&pSwapChainFramebuffers, device, renderPass,
                            swapChainExtent, swapChainImageCount,
                            pSwapChainImageViews);

  VkCommandPool commandPool;
  new_CommandPool(&commandPool, device, graphicsIndex);
#define VERTEXNUM 6
  struct Vertex vertices[VERTEXNUM] = {
      {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}}, {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
      {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},

      {{0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}}, {{1.0f, 0.5f}, {0.0f, 1.0f, 1.0f}},
      {{0.0f, 0.5f}, {1.0f, 1.0f, 0.0f}},
  };

  VkBuffer vertexBuffer;
  VkDeviceMemory vertexBufferMemory;
  new_VertexBuffer(&vertexBuffer, &vertexBufferMemory, vertices, VERTEXNUM,
                   device, physicalDevice, commandPool, graphicsQueue);

  VkDescriptorPool modelViewProjectionDescriptorPool;
  new_ModelViewProjectionDescriptorPool(&modelViewProjectionDescriptorPool,
                                        swapChainImageCount, device);

  VkBuffer *pModelViewProjectionUniformBuffers;
  VkDeviceMemory *pModelViewProjectionUniformBufferMemories;
  new_ModelViewProjectionUniformBuffers(
      &pModelViewProjectionUniformBuffers,
      &pModelViewProjectionUniformBufferMemories, swapChainImageCount,
      physicalDevice, device);

  VkDescriptorSet *pModelViewProjectionDescriptorSets;
  new_ModelViewProjectionDescriptorSets(
      &pModelViewProjectionDescriptorSets, pModelViewProjectionUniformBuffers,
      swapChainImageCount, modelViewProjectionDescriptorSetLayout,
      modelViewProjectionDescriptorPool, device);

  VkCommandBuffer *pVertexDisplayCommandBuffers;
  new_VertexDisplayCommandBuffers(
      &pVertexDisplayCommandBuffers, vertexBuffer, VERTEXNUM, device,
      renderPass, vertexDisplayPipelineLayout, vertexDisplayPipeline,
      commandPool, swapChainExtent, swapChainImageCount,
      pModelViewProjectionDescriptorSets, pSwapChainFramebuffers);

  VkSemaphore *pImageAvailableSemaphores;
  VkSemaphore *pRenderFinishedSemaphores;
  VkFence *pInFlightFences;
  new_Semaphores(&pImageAvailableSemaphores, swapChainImageCount, device);
  new_Semaphores(&pRenderFinishedSemaphores, swapChainImageCount, device);
  new_Fences(&pInFlightFences, swapChainImageCount, device);

  struct ModelViewProjectionMatrices cameraView;
  mat4x4_identity(cameraView.model);
  mat4x4_identity(cameraView.view);
  mat4x4_identity(cameraView.projection);

  uint32_t currentFrame = 0;
  /*wait till close*/
  while (!glfwWindowShouldClose(pWindow)) {
    glfwPollEvents();

    mat4x4_look_at(cameraView.view, (vec3){2.0f, 2.0f, 2.0f}, (vec3){0.0f, 0.0f, 0.0f}, (vec3){0.0f, 0.0f, 1.0f});
    mat4x4_perspective(cameraView.projection, 1, ((float)swapChainExtent.width)/swapChainExtent.height, 0.1f, 10.0f); /* The 1 is in radians */
    mat4x4_rotate_Z(cameraView.model, cameraView.model, 0.01);

    ErrVal result = drawFrame(
        &currentFrame, 2, pModelViewProjectionUniformBufferMemories, cameraView, device, swapChain, pVertexDisplayCommandBuffers,
        pInFlightFences, pImageAvailableSemaphores, pRenderFinishedSemaphores,
        graphicsQueue, presentQueue);


    if (result == ERR_OUTOFDATE) {
      vkDeviceWaitIdle(device);
      delete_Fences(&pInFlightFences, swapChainImageCount, device);
      delete_Semaphores(&pRenderFinishedSemaphores, swapChainImageCount,
                        device);
      delete_Semaphores(&pImageAvailableSemaphores, swapChainImageCount,
                        device);
      delete_CommandBuffers(&pVertexDisplayCommandBuffers);
      delete_CommandPool(&commandPool, device);
      delete_SwapChainFramebuffers(&pSwapChainFramebuffers, swapChainImageCount,
                                   device);
      delete_Pipeline(&vertexDisplayPipeline, device);
      delete_PipelineLayout(&vertexDisplayPipelineLayout, device);
      delete_RenderPass(&renderPass, device);
      delete_SwapChainImageViews(&pSwapChainImageViews, swapChainImageCount,
                                 device);
      delete_SwapChainImages(&pSwapChainImages);
      delete_SwapChain(&swapChain, device);

      /* Set swapchain to new window size */
      getWindowExtent(&swapChainExtent, pWindow);

      /*Create swap chain */
      new_SwapChain(&swapChain, &swapChainImageCount, VK_NULL_HANDLE,
                    surfaceFormat, physicalDevice, device, surface,
                    swapChainExtent, graphicsIndex, presentIndex);
      new_SwapChainImages(&pSwapChainImages, &swapChainImageCount, device,
                          swapChain);
      new_SwapChainImageViews(&pSwapChainImageViews, device,
                              surfaceFormat.format, swapChainImageCount,
                              pSwapChainImages);

      /* Create graphics pipeline */
      new_RenderPass(&renderPass, device, surfaceFormat.format);
      new_VertexDisplayPipelineLayout(&vertexDisplayPipelineLayout,
                                      modelViewProjectionDescriptorSetLayout,
                                      device);
      new_VertexDisplayPipeline(
          &vertexDisplayPipeline, device, vertShaderModule, fragShaderModule,
          swapChainExtent, renderPass, vertexDisplayPipelineLayout);
      new_SwapChainFramebuffers(&pSwapChainFramebuffers, device, renderPass,
                                swapChainExtent, swapChainImageCount,
                                pSwapChainImageViews);
      new_CommandPool(&commandPool, device, graphicsIndex);

      new_VertexDisplayCommandBuffers(
          &pVertexDisplayCommandBuffers, vertexBuffer, VERTEXNUM, device,
          renderPass, vertexDisplayPipelineLayout, vertexDisplayPipeline,
          commandPool, swapChainExtent, swapChainImageCount,
          pModelViewProjectionDescriptorSets, pSwapChainFramebuffers);
      new_Semaphores(&pImageAvailableSemaphores, swapChainImageCount, device);
      new_Semaphores(&pRenderFinishedSemaphores, swapChainImageCount, device);
      new_Fences(&pInFlightFences, swapChainImageCount, device);
    }
  }

  /*cleanup*/
  vkDeviceWaitIdle(device);
  delete_ShaderModule(&fragShaderModule, device);
  delete_ShaderModule(&vertShaderModule, device);
  delete_Fences(&pInFlightFences, swapChainImageCount, device);
  delete_Semaphores(&pRenderFinishedSemaphores, swapChainImageCount, device);
  delete_Semaphores(&pImageAvailableSemaphores, swapChainImageCount, device);
  delete_CommandBuffers(&pVertexDisplayCommandBuffers);
  delete_CommandPool(&commandPool, device);
  delete_DescriptorSets(&pModelViewProjectionDescriptorSets);
  delete_ModelViewProjectionUniformBuffers(
      &pModelViewProjectionUniformBuffers,
      &pModelViewProjectionUniformBufferMemories, swapChainImageCount, device);
  delete_DescriptorPool(&modelViewProjectionDescriptorPool, device);
  delete_DescriptorSetLayout(&modelViewProjectionDescriptorSetLayout, device);
  delete_SwapChainFramebuffers(&pSwapChainFramebuffers, swapChainImageCount,
                               device);
  delete_Pipeline(&vertexDisplayPipeline, device);
  delete_PipelineLayout(&vertexDisplayPipelineLayout, device);
  delete_Buffer(&vertexBuffer, device);
  delete_DeviceMemory(&vertexBufferMemory, device);
  delete_RenderPass(&renderPass, device);
  delete_SwapChainImageViews(&pSwapChainImageViews, swapChainImageCount,
                             device);
  delete_SwapChainImages(&pSwapChainImages);
  delete_SwapChain(&swapChain, device);
  delete_Device(&device);
  delete_Surface(&surface, instance);
  delete_DebugCallback(&callback, instance);
  delete_Instance(&instance);
  delete_RequiredDeviceExtensions(&deviceExtensionCount,
                                  &ppDeviceExtensionNames);
  delete_ValidationLayers(&layerCount, &ppLayerNames);
  delete_RequiredInstanceExtensions(&extensionCount, &ppExtensionNames);
  glfwTerminate();
  return (EXIT_SUCCESS);
}
