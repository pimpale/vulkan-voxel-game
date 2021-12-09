#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define APPNAME "Vulkan Triangle"

#include "camera.h"
#include "utils.h"
#include "vulkan_utils.h"
#include "worldgen.h"

#include "errors.h"

#define WINDOW_HEIGHT 500
#define WINDOW_WIDTH 500
#define MAX_FRAMES_IN_FLIGHT 2

// contins state associated with the vulkan instance
typedef struct {
  VkInstance instance;
  VkDebugUtilsMessengerEXT callback;
  VkPhysicalDevice physicalDevice;
  GLFWwindow *pWindow;
  VkSurfaceKHR surface;
  VkSurfaceFormatKHR surfaceFormat;
  uint32_t graphicsIndex;
  uint32_t computeIndex;
  uint32_t presentIndex;
  VkQueue graphicsQueue;
  VkQueue presentQueue;
  VkDevice device;
  VkCommandPool commandPool;
  // shaders, we need them to recreate the graphics pipeline
  VkShaderModule fragShaderModule;
  VkShaderModule vertShaderModule;
  VkRenderPass renderPass;
  VkPipelineLayout graphicsPipelineLayout;
  VkCommandBuffer pVertexDisplayCommandBuffers[MAX_FRAMES_IN_FLIGHT];
  VkSemaphore pImageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];
  VkSemaphore pRenderFinishedSemaphores[MAX_FRAMES_IN_FLIGHT];
  VkFence pInFlightFences[MAX_FRAMES_IN_FLIGHT];
  // this number counts which frame we're on
  // up to MAX_FRAMES_IN_FLIGHT, at whcich points it resets to 0
  uint32_t currentFrame;
} AppGraphicsGlobalState;

void new_AppGraphicsGlobalState(AppGraphicsGlobalState *pGlobal) {
  glfwInit();

  const uint32_t validationLayerCount = 1;
  const char *ppValidationLayerNames[1] = {"VK_LAYER_KHRONOS_validation"};

  /* Create pGlobal->instance */
  new_Instance(&pGlobal->instance, validationLayerCount, ppValidationLayerNames,
               0, NULL, true, true, APPNAME);

  /* Enable vulkan logging to stdout */
  new_DebugCallback(&pGlobal->callback, pGlobal->instance);

  /* get physical pGlobal->device */
  getPhysicalDevice(&pGlobal->physicalDevice, pGlobal->instance);

  /* Create window and pGlobal->surface */
  new_GlfwWindow(&pGlobal->pWindow, APPNAME,
                 (VkExtent2D){.width = WINDOW_WIDTH, .height = WINDOW_HEIGHT});
  new_SurfaceFromGLFW(&pGlobal->surface, pGlobal->pWindow, pGlobal->instance);

  /* find queues on graphics pGlobal->device */
  {
    uint32_t ret1 = getQueueFamilyIndexByCapability(&pGlobal->graphicsIndex,
                                                    pGlobal->physicalDevice,
                                                    VK_QUEUE_GRAPHICS_BIT);
    uint32_t ret3 = getPresentQueueFamilyIndex(
        &pGlobal->presentIndex, pGlobal->physicalDevice, pGlobal->surface);
    /* Panic if indices are unavailable */
    if (ret1 != VK_SUCCESS || ret3 != VK_SUCCESS) {
      LOG_ERROR(ERR_LEVEL_FATAL, "unable to acquire indices\n");
      PANIC();
    }
  }

  // we want to use swapchains to reduce tearing
  const uint32_t deviceExtensionCount = 1;
  const char *ppDeviceExtensionNames[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

  // create pGlobal->device
  new_Device(&pGlobal->device, pGlobal->physicalDevice, pGlobal->graphicsIndex,
             deviceExtensionCount, ppDeviceExtensionNames);

  // create queues
  getQueue(&pGlobal->graphicsQueue, pGlobal->device, pGlobal->graphicsIndex);
  getQueue(&pGlobal->presentQueue, pGlobal->device, pGlobal->presentIndex);

  // We can create command buffers from the command pool
  new_CommandPool(&pGlobal->commandPool, pGlobal->device,
                  pGlobal->graphicsIndex);

  // get preferred format of screen
  getPreferredSurfaceFormat(&pGlobal->surfaceFormat, pGlobal->physicalDevice,
                            pGlobal->surface);

  {
    uint32_t *fragShaderFileContents;
    uint32_t fragShaderFileLength;
    readShaderFile("assets/shaders/shader.frag.spv", &fragShaderFileLength,
                   &fragShaderFileContents);
    new_ShaderModule(&pGlobal->fragShaderModule, pGlobal->device,
                     fragShaderFileLength, fragShaderFileContents);
    free(fragShaderFileContents);
  }

  {
    uint32_t *vertShaderFileContents;
    uint32_t vertShaderFileLength;
    readShaderFile("assets/shaders/shader.vert.spv", &vertShaderFileLength,
                   &vertShaderFileContents);
    new_ShaderModule(&pGlobal->vertShaderModule, pGlobal->device,
                     vertShaderFileLength, vertShaderFileContents);
    free(vertShaderFileContents);
  }

  // Create graphics pipeline
  new_VertexDisplayRenderPass(&pGlobal->renderPass, pGlobal->device,
                              pGlobal->surfaceFormat.format);

  new_VertexDisplayPipelineLayout(&pGlobal->graphicsPipelineLayout,
                                  pGlobal->device);

  new_CommandBuffers(pGlobal->pVertexDisplayCommandBuffers, 2,
                     pGlobal->commandPool, pGlobal->device);

  // Create image synchronization primitives
  new_Semaphores(pGlobal->pImageAvailableSemaphores, MAX_FRAMES_IN_FLIGHT,
                 pGlobal->device);
  new_Semaphores(pGlobal->pRenderFinishedSemaphores, MAX_FRAMES_IN_FLIGHT,
                 pGlobal->device);
  // fences start off signaled
  new_Fences(pGlobal->pInFlightFences, MAX_FRAMES_IN_FLIGHT, pGlobal->device,
             true);
  // set current frame to 0
  pGlobal->currentFrame = 0;
}

static void delete_GraphicsGlobalState(AppGraphicsGlobalState *pGlobal) {
  vkDeviceWaitIdle(pGlobal->device);
  delete_ShaderModule(&pGlobal->fragShaderModule, pGlobal->device);
  delete_ShaderModule(&pGlobal->vertShaderModule, pGlobal->device);

  delete_Fences(pGlobal->pInFlightFences, MAX_FRAMES_IN_FLIGHT,
                pGlobal->device);
  delete_Semaphores(pGlobal->pRenderFinishedSemaphores, MAX_FRAMES_IN_FLIGHT,
                    pGlobal->device);
  delete_Semaphores(pGlobal->pImageAvailableSemaphores, MAX_FRAMES_IN_FLIGHT,
                    pGlobal->device);

  delete_CommandBuffers(pGlobal->pVertexDisplayCommandBuffers,
                        MAX_FRAMES_IN_FLIGHT, pGlobal->commandPool,
                        pGlobal->device);
  delete_CommandPool(&pGlobal->commandPool, pGlobal->device);

  delete_PipelineLayout(&pGlobal->graphicsPipelineLayout, pGlobal->device);
  delete_RenderPass(&pGlobal->renderPass, pGlobal->device);
  delete_Device(&pGlobal->device);
  delete_Surface(&pGlobal->surface, pGlobal->instance);
  delete_DebugCallback(&pGlobal->callback, pGlobal->instance);
  delete_Instance(&pGlobal->instance);
  glfwTerminate();
}

// contains state associated with the window that must be resized
typedef struct {
  VkSwapchainKHR swapchain;
  uint32_t swapchainImageCount;
  VkImage *pSwapchainImages;
  VkImageView *pSwapchainImageViews;
  VkDeviceMemory depthImageMemory;
  VkImage depthImage;
  VkImageView depthImageView;
  VkFramebuffer *pSwapchainFramebuffers;
  VkPipeline graphicsPipeline;
  VkExtent2D swapchainExtent;
} AppGraphicsWindowState;

// contains the per app state that we don't need to resize

void new_AppGraphicsWindowState(           //
    AppGraphicsWindowState *pWindow,       //
    const AppGraphicsGlobalState *pGlobal, //
    const VkExtent2D swapchainExtent       //
) {

  // Create swap chain
  new_Swapchain(                     //
      &pWindow->swapchain,           //
      &pWindow->swapchainImageCount, //
      VK_NULL_HANDLE,                //
      pGlobal->surfaceFormat,        //
      pGlobal->physicalDevice,       //
      pGlobal->device,               //
      pGlobal->surface,              //
      swapchainExtent,               //
      pGlobal->graphicsIndex,        //
      pGlobal->presentIndex          //
  );

  // there are swapchainImageCount swapchainImages
  pWindow->pSwapchainImages =
      malloc(pWindow->swapchainImageCount * sizeof(VkImage));
  getSwapchainImages(pWindow->pSwapchainImages, pWindow->swapchainImageCount,
                     pGlobal->device, pWindow->swapchain);

  // there are swapchainImageCount swapchainImageViews
  pWindow->pSwapchainImageViews =
      malloc(pWindow->swapchainImageCount * sizeof(VkImageView));
  new_SwapchainImageViews(pWindow->pSwapchainImageViews,
                          pWindow->pSwapchainImages,
                          pWindow->swapchainImageCount, pGlobal->device,
                          pGlobal->surfaceFormat.format);

  // create depth buffer
  new_DepthImage(&pWindow->depthImage, &pWindow->depthImageMemory,
                 swapchainExtent, pGlobal->physicalDevice, pGlobal->device);
  new_DepthImageView(&pWindow->depthImageView, pGlobal->device,
                     pWindow->depthImage);

  // create framebuffers to render to
  pWindow->pSwapchainFramebuffers =
      malloc(pWindow->swapchainImageCount * sizeof(VkFramebuffer));
  new_SwapchainFramebuffers(
      pWindow->pSwapchainFramebuffers, pGlobal->device, pGlobal->renderPass,
      swapchainExtent, pWindow->swapchainImageCount, pWindow->depthImageView,
      pWindow->pSwapchainImageViews);

  // create pipeline
  new_VertexDisplayPipeline(
      &pWindow->graphicsPipeline, pGlobal->device, pGlobal->vertShaderModule,
      pGlobal->fragShaderModule, swapchainExtent, pGlobal->renderPass,
      pGlobal->graphicsPipelineLayout);

  // remember swapchain extent
  pWindow->swapchainExtent = swapchainExtent;
}

void delete_AppGraphicsWindowState(AppGraphicsWindowState *pWindow,
                                   const AppGraphicsGlobalState *pGlobal) {
  vkDeviceWaitIdle(pGlobal->device);

  // delete framebuffers
  delete_SwapchainFramebuffers(pWindow->pSwapchainFramebuffers,
                               pWindow->swapchainImageCount, pGlobal->device);
  free(pWindow->pSwapchainFramebuffers);
  // delete swapchain imageviews
  delete_SwapchainImageViews(pWindow->pSwapchainImageViews,
                             pWindow->swapchainImageCount, pGlobal->device);
  free(pWindow->pSwapchainImageViews);
  // note that we don't have to delete swapchain images since they go away with
  // the swapchain
  free(pWindow->pSwapchainImages);
  // delete swapchain
  delete_Swapchain(&pWindow->swapchain, pGlobal->device);
  // delete depth buffer
  delete_ImageView(&pWindow->depthImageView, pGlobal->device);
  delete_Image(&pWindow->depthImage, pGlobal->device);
  delete_DeviceMemory(&pWindow->depthImageMemory, pGlobal->device);

  // delete pipeline
  delete_Pipeline(&pWindow->graphicsPipeline, pGlobal->device);
}

void drawAppFrame(                    //
    AppGraphicsWindowState *pWindow, //
    AppGraphicsGlobalState *pGlobal, //
    Camera *pCamera,                 //
    uint32_t vertexCount,            //
    VkBuffer vertexBuffer            //

) {
  glfwPollEvents();

  // wait for last frame to finish
  waitAndResetFence(pGlobal->pInFlightFences[pGlobal->currentFrame],
                    pGlobal->device);

  // the imageIndex is the index of the swapchain framebuffer that is
  // available next
  uint32_t imageIndex;
  // this function will return immediately,
  //  so we use the semaphore to tell us when the image is actually available,
  //  (ready for rendering to)
  ErrVal result = getNextSwapchainImage(
      &imageIndex, pWindow->swapchain, pGlobal->device,
      pGlobal->pImageAvailableSemaphores[pGlobal->currentFrame]);

  // if the window is resized
  if (result == ERR_OUTOFDATE) {
    // get new window size
    VkExtent2D swapchainExtent;
    getExtentWindow(&swapchainExtent, pGlobal->pWindow);

    // resize camera
    resizeCamera(pCamera, swapchainExtent);

    // destroy and recreate window dependent data
    delete_AppGraphicsWindowState(pWindow, pGlobal);
    new_AppGraphicsWindowState(pWindow, pGlobal, swapchainExtent);

    // finally we can retry getting the swapchain
    getNextSwapchainImage(
        &imageIndex, pWindow->swapchain, pGlobal->device,
        pGlobal->pImageAvailableSemaphores[pGlobal->currentFrame]);
  }

  // update camera
  updateCamera(pCamera, pGlobal->pWindow);
  mat4x4 mvp;
  getMvpCamera(mvp, pCamera);

  // record buffer
  recordVertexDisplayCommandBuffer(                                 //
      pGlobal->pVertexDisplayCommandBuffers[pGlobal->currentFrame], //
      pWindow->pSwapchainFramebuffers[imageIndex],                  //
      vertexBuffer,                                                 //
      vertexCount,                                                  //
      pGlobal->renderPass,                                          //
      pGlobal->graphicsPipelineLayout,                              //
      pWindow->graphicsPipeline,                                    //
      pWindow->swapchainExtent,                                     //
      mvp,                                                          //
      (VkClearColorValue){.float32 = {0, 0, 0, 0}}                  //
  );

  drawFrame(                                                        //
      pGlobal->pVertexDisplayCommandBuffers[pGlobal->currentFrame], //
      pWindow->swapchain,                                           //
      imageIndex,                                                   //
      pGlobal->pImageAvailableSemaphores[pGlobal->currentFrame],    //
      pGlobal->pRenderFinishedSemaphores[pGlobal->currentFrame],    //
      pGlobal->pInFlightFences[pGlobal->currentFrame],              //
      pGlobal->graphicsQueue,                                       //
      pGlobal->presentQueue                                         //
  );

  // increment frame
  pGlobal->currentFrame = (pGlobal->currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

int main(void) {
  // initialize global graphics
  AppGraphicsGlobalState global;
  new_AppGraphicsGlobalState(&global);

  // set up world generation
  WorldState ws;
  wg_new_WorldState_(&ws, (ivec3){0, 0, 0}, 0);

  // calc number of vertexes
  uint32_t vertexCount;
  wg_world_count_vertexes(&vertexCount, &ws);

  // no offset
  vec3 offset;
  Vertex *vertexData = malloc(vertexCount * sizeof(Vertex));
  wg_world_mesh(vertexData, offset, &ws);

  VkBuffer vertexBuffer;
  VkDeviceMemory vertexBufferMemory;
  new_VertexBuffer(&vertexBuffer, &vertexBufferMemory, vertexData, vertexCount,
                   global.device, global.physicalDevice, global.commandPool,
                   global.graphicsQueue);

  // Set extent (window width and height)
  VkExtent2D swapchainExtent;
  getExtentWindow(&swapchainExtent, global.pWindow);

  // create window graphics
  AppGraphicsWindowState window;
  new_AppGraphicsWindowState(&window, &global, swapchainExtent);

  // create camera
  vec3 loc = {0.0f, 0.0f, 0.0f};
  Camera camera = new_Camera(loc, swapchainExtent);

  /*wait till close*/
  while (!glfwWindowShouldClose(global.pWindow)) {
    drawAppFrame(&window, &global, &camera, vertexCount, vertexBuffer);
  }

  // delete our buffer
  delete_Buffer(&vertexBuffer, global.device);
  delete_DeviceMemory(&vertexBufferMemory, global.device);

  // delete graphics resources
  delete_AppGraphicsWindowState(&window, &global);
  delete_GraphicsGlobalState(&global);

  return (EXIT_SUCCESS);
}
