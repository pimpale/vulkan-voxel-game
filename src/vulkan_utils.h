/*
 * vulkan_methods.h
 *
 *  Created on: Aug 8, 2018
 *      Author: gpi
 */

#ifndef VULKAN_HELPER_H_
#define VULKAN_HELPER_H_

#include <stdint.h>
#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "linmath.h"

#include "errors.h"

struct Vertex {
  vec3 position;
  vec3 color;
};

char *vkstrerror(VkResult err);

ErrVal new_RequiredInstanceExtensions(uint32_t *pEnabledExtensionCount,
                                      char ***pppEnabledExtensionNames);

void delete_RequiredInstanceExtensions(uint32_t *pEnabledExtensionCount,
                                       char ***pppEnabledExtensionNames);

ErrVal new_ValidationLayers(uint32_t *pLayerCount, char ***pppLayerNames);

void delete_ValidationLayers(uint32_t *pLayerCount, char ***pppLayerNames);

ErrVal new_RequiredDeviceExtensions(uint32_t *pEnabledExtensionCount,
                                    char ***pppEnabledExtensionNames);

void delete_RequiredDeviceExtensions(uint32_t *pEnabledExtensionCount,
                                     char ***pppEnabledExtensionNames);

ErrVal new_Instance(VkInstance *pInstance, const uint32_t enabledExtensionCount,
                    const char *const *ppEnabledExtensionNames,
                    const uint32_t enabledLayerCount,
                    const char *const *ppEnabledLayerNames);

void delete_Instance(VkInstance *pInstance);

ErrVal new_GLFWwindow(GLFWwindow **ppGLFWwindow);

ErrVal getWindowExtent(VkExtent2D *pExtent, GLFWwindow *pWindow);

ErrVal new_DebugCallback(VkDebugUtilsMessengerEXT *pCallback,
                         const VkInstance instance);

void delete_DebugCallback(VkDebugUtilsMessengerEXT *pCallback,
                          const VkInstance instance);

ErrVal getPhysicalDevice(VkPhysicalDevice *pDevice, const VkInstance instance);

ErrVal new_Device(VkDevice *pDevice, const VkPhysicalDevice physicalDevice,
                  const uint32_t deviceQueueIndex,
                  const uint32_t enabledExtensionCount,
                  const char *const *ppEnabledExtensionNames,
                  const uint32_t enabledLayerCount,
                  const char *const *ppEnabledLayerNames);

void delete_Device(VkDevice *pDevice);

ErrVal getDeviceQueueIndex(uint32_t *deviceQueueIndex,
                           const VkPhysicalDevice device,
                           const VkQueueFlags bit);

ErrVal getPresentQueueIndex(uint32_t *pPresentQueueIndex,
                            const VkPhysicalDevice physicalDevice,
                            const VkSurfaceKHR surface);

ErrVal getQueue(VkQueue *pQueue, const VkDevice device,
                const uint32_t deviceQueueIndex);

ErrVal getPreferredSurfaceFormat(VkSurfaceFormatKHR *pSurfaceFormat,
                                 const VkPhysicalDevice physicalDevice,
                                 const VkSurfaceKHR surface);

ErrVal new_SwapChain(VkSwapchainKHR *pSwapChain, uint32_t *pSwapChainImageCount,
                     const VkSwapchainKHR oldSwapChain,
                     const VkSurfaceFormatKHR surfaceFormat,
                     const VkPhysicalDevice physicalDevice,
                     const VkDevice device, const VkSurfaceKHR surface,
                     const VkExtent2D extent, const uint32_t graphicsIndex,
                     const uint32_t presentIndex);

void delete_SwapChain(VkSwapchainKHR *pSwapChain, const VkDevice device);

ErrVal new_SwapChainImages(VkImage **ppSwapChainImages, uint32_t *pImageCount,
                           const VkDevice device,
                           const VkSwapchainKHR swapChain);

void delete_SwapChainImages(VkImage **ppImages);

ErrVal new_SwapChainImageViews(VkImageView **ppImageViews,
                               const VkDevice device, const VkFormat format,
                               const uint32_t imageCount,
                               const VkImage *pSwapChainImages);

void delete_SwapChainImageViews(VkImageView **ppImageViews, uint32_t imageCount,
                                const VkDevice device);

ErrVal new_ShaderModule(VkShaderModule *pShaderModule, const VkDevice device,
                        const uint32_t codeSize, const uint32_t *pCode);

ErrVal new_ShaderModuleFromFile(VkShaderModule *pShaderModule,
                                const VkDevice device, char *filename);

void delete_ShaderModule(VkShaderModule *pShaderModule, const VkDevice device);

ErrVal new_RenderPass(VkRenderPass *pRenderPass, const VkDevice device,
                      const VkFormat swapChainImageFormat);

void delete_RenderPass(VkRenderPass *pRenderPass, const VkDevice device);

ErrVal new_VertexDisplayPipelineLayout(VkPipelineLayout *pPipelineLayout,
                                       const VkDevice device);

void delete_PipelineLayout(VkPipelineLayout *pPipelineLayout,
                           const VkDevice device);

ErrVal new_VertexDisplayPipeline(VkPipeline *pVertexDisplayPipeline,
                                 const VkDevice device,
                                 const VkShaderModule vertShaderModule,
                                 const VkShaderModule fragShaderModule,
                                 const VkExtent2D extent,
                                 const VkRenderPass renderPass,
                                 const VkPipelineLayout pipelineLayout);

void delete_Pipeline(VkPipeline *pPipeline, const VkDevice device);

ErrVal new_SwapChainFramebuffers(VkFramebuffer **ppFramebuffers,
                                 const VkDevice device,
                                 const VkRenderPass renderPass,
                                 const VkExtent2D swapChainExtent,
                                 const uint32_t imageCount,
                                 const VkImageView *pSwapChainImageViews);

void delete_SwapChainFramebuffers(VkFramebuffer **ppFramebuffers,
                                  const uint32_t imageCount,
                                  const VkDevice device);

ErrVal new_CommandPool(VkCommandPool *pCommandPool, const VkDevice device,
                       const uint32_t queueFamilyIndex);

void delete_CommandPool(VkCommandPool *pCommandPool, const VkDevice device);


ErrVal new_VertexDisplayCommandBuffers(
    VkCommandBuffer **ppCommandBuffers, const VkBuffer vertexBuffer,
    const uint32_t vertexCount, const VkDevice device,
    const VkRenderPass renderPass,
	const VkPipelineLayout vertexDisplayPipelineLayout,
	const VkPipeline vertexDisplayPipeline,
    const VkCommandPool commandPool, const VkExtent2D swapChainExtent,
    const uint32_t swapChainFramebufferCount,
    const VkFramebuffer *pSwapChainFramebuffers,
	const mat4x4 cameraTransform);

void delete_CommandBuffers(VkCommandBuffer **ppCommandBuffers);

ErrVal new_Semaphore(VkSemaphore *pSemaphore, const VkDevice device);

void delete_Semaphore(VkSemaphore *pSemaphore, const VkDevice device);

ErrVal new_Semaphores(VkSemaphore **ppSemaphores, const uint32_t semaphoreCount,
                      const VkDevice device);

void delete_Semaphores(VkSemaphore **ppSemaphores,
                       const uint32_t semaphoreCount, const VkDevice device);

ErrVal new_Fences(VkFence **ppFences, const uint32_t fenceCount,
                  const VkDevice device);

void delete_Fences(VkFence **ppFences, const uint32_t fenceCount,
                   const VkDevice device);

ErrVal drawFrame(uint32_t *pCurrentFrame, const uint32_t maxFramesInFlight,
                 const VkDevice device, const VkSwapchainKHR swapChain,
                 const VkCommandBuffer *pCommandBuffers,
                 const VkFence *pInFlightFences,
                 const VkSemaphore *pImageAvailableSemaphores,
                 const VkSemaphore *pRenderFinishedSemaphores,
                 const VkQueue graphicsQueue, const VkQueue presentQueue);

ErrVal new_SurfaceFromGLFW(VkSurfaceKHR *pSurface, GLFWwindow *pWindow,
                           const VkInstance instance);

void delete_Surface(VkSurfaceKHR *pSurface, const VkInstance instance);

ErrVal new_Buffer_DeviceMemory(VkBuffer *pBuffer, VkDeviceMemory *pBufferMemory,
                               const VkDeviceSize size,
                               const VkPhysicalDevice physicalDevice,
                               const VkDevice device,
                               const VkBufferUsageFlags usage,
                               const VkMemoryPropertyFlags properties);

ErrVal new_VertexBuffer(VkBuffer *pBuffer, VkDeviceMemory *pBufferMemory,
                        const struct Vertex *pVertices,
                        const uint32_t vertexCount, const VkDevice device,
                        const VkPhysicalDevice physicalDevice,
                        const VkCommandPool commandPool, const VkQueue queue);

ErrVal copyBuffer(VkBuffer destinationBuffer, const VkBuffer sourceBuffer,
                  const VkDeviceSize size, const VkCommandPool commandPool,
                  const VkQueue queue, const VkDevice device);

void delete_Buffer(VkBuffer *pBuffer, const VkDevice device);

void delete_DeviceMemory(VkDeviceMemory *pDeviceMemory, const VkDevice device);

ErrVal new_begin_OneTimeSubmitCommandBuffer(VkCommandBuffer *pCommandBuffer,
                                            const VkDevice device,
                                            const VkCommandPool commandPool);

ErrVal delete_end_OneTimeSubmitCommandBuffer(VkCommandBuffer *pCommandBuffer,
                                             const VkDevice device,
                                             const VkQueue queue,
                                             const VkCommandPool commandPool);

#endif /* VULKAN_HELPER_H_ */
