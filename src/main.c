#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define APPNAME "BlockRender"

#include "block.h"
#include "camera.h"
#include "utils.h"
#include "vulkan_utils.h"
#include "world.h"

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
  uint32_t graphicsQueueCount;
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
  VkDescriptorSetLayout graphicsDescriptorSetLayout;
  // descriptor pool stuff
  VkDescriptorPool graphicsDescriptorPool;
  VkDescriptorSet graphicsDescriptorSet;
  VkImage textureAtlasImage;
  VkDeviceMemory textureAtlasImageMemory;
  VkImageView textureAtlasImageView;
  VkSampler textureAtlasSampler;
  VkCommandBuffer pVertexDisplayCommandBuffers[MAX_FRAMES_IN_FLIGHT];
  VkSemaphore pImageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];
  VkSemaphore pRenderFinishedSemaphores[MAX_FRAMES_IN_FLIGHT];
  VkFence pInFlightFences[MAX_FRAMES_IN_FLIGHT];
  // this number counts which frame we're on
  // up to MAX_FRAMES_IN_FLIGHT, at whcich points it resets to 0
  uint32_t currentFrame;
} AppGraphicsGlobalState;

static void new_AppGraphicsGlobalState(AppGraphicsGlobalState *pGlobal) {
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

  // configure glfw
  glfwSetInputMode(pGlobal->pWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  glfwSetInputMode(pGlobal->pWindow, GLFW_STICKY_MOUSE_BUTTONS, GLFW_TRUE);

  new_SurfaceFromGLFW(&pGlobal->surface, pGlobal->pWindow, pGlobal->instance);

  /* find queues on graphics pGlobal->device */
  {
    uint32_t ret1 = getQueueFamilyIndexByCapability(
        &pGlobal->graphicsIndex, &pGlobal->graphicsQueueCount,
        pGlobal->physicalDevice, VK_QUEUE_GRAPHICS_BIT);
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
             pGlobal->graphicsQueueCount, deviceExtensionCount,
             ppDeviceExtensionNames);

  // create queues
  getQueue(&pGlobal->graphicsQueue, pGlobal->device, pGlobal->graphicsIndex, 0);
  getQueue(&pGlobal->presentQueue, pGlobal->device, pGlobal->presentIndex, 0);

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

  // create texture and samplers
  uint8_t textureAtlasData[BLOCK_TEXTURE_ATLAS_LEN];
  block_buildTextureAtlas(textureAtlasData, "assets/blocks");

  new_TextureImage(                                      //
      &pGlobal->textureAtlasImage,                       //
      &pGlobal->textureAtlasImageMemory,                 //
      textureAtlasData,                                  //
      (VkExtent2D){.height = BLOCK_TEXTURE_ATLAS_HEIGHT, //
                   .width = BLOCK_TEXTURE_ATLAS_WIDTH},  //
      pGlobal->device,                                   //
      pGlobal->physicalDevice,                           //
      pGlobal->commandPool,                              //
      pGlobal->graphicsQueue                             //
  );
  new_TextureImageView(&pGlobal->textureAtlasImageView,
                       pGlobal->textureAtlasImage, pGlobal->device);
  new_TextureSampler(&pGlobal->textureAtlasSampler, pGlobal->device);

  // Create graphics pipeline layout
  new_VertexDisplayRenderPass(&pGlobal->renderPass, pGlobal->device,
                              pGlobal->surfaceFormat.format);

  new_VertexDisplayPipelineLayoutDescriptorSetLayout(
      &pGlobal->graphicsPipelineLayout, &pGlobal->graphicsDescriptorSetLayout,
      pGlobal->device);

  // create descriptor set using the texture and the descriptor set layout
  new_VertexDisplayDescriptorPoolAndSet(    //
      &pGlobal->graphicsDescriptorPool,     //
      &pGlobal->graphicsDescriptorSet,      //
      pGlobal->graphicsDescriptorSetLayout, //
      pGlobal->device,                      //
      pGlobal->textureAtlasSampler,         //
      pGlobal->textureAtlasImageView        //
  );

  new_CommandBuffers(pGlobal->pVertexDisplayCommandBuffers,
                     MAX_FRAMES_IN_FLIGHT, pGlobal->commandPool,
                     pGlobal->device);

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

  delete_DescriptorPool(&pGlobal->graphicsDescriptorPool, pGlobal->device);
  delete_TextureSampler(&pGlobal->textureAtlasSampler, pGlobal->device);
  delete_ImageView(&pGlobal->textureAtlasImageView, pGlobal->device);
  delete_Image(&pGlobal->textureAtlasImage, pGlobal->device);
  delete_DeviceMemory(&pGlobal->textureAtlasImageMemory, pGlobal->device);

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

  delete_VertexDisplayPipelineLayoutDescriptorSetLayout(
      &pGlobal->graphicsPipelineLayout, &pGlobal->graphicsDescriptorSetLayout,
      pGlobal->device);
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

static void new_AppGraphicsWindowState(    //
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

static void
delete_AppGraphicsWindowState(AppGraphicsWindowState *pWindow,
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

static void drawAppFrame(            //
    AppGraphicsWindowState *pWindow, //
    AppGraphicsGlobalState *pGlobal, //
    Camera *pCamera,                 //
    WorldState *pWs                  //
) {
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

  uint32_t vertexBufferCount;
  wld_count_vertexBuffers(&vertexBufferCount, pWs);

  VkBuffer *pVertexBuffers = malloc(vertexBufferCount * sizeof(VkBuffer));
  uint32_t *pVertexCounts = malloc(vertexBufferCount * sizeof(uint32_t));

  wld_getVertexBuffers(pVertexBuffers, pVertexCounts, pWs);

  mat4x4 mvp;
  getMvpCamera(mvp, pCamera);

  // record buffer
  recordVertexDisplayCommandBuffer(                                 //
      pGlobal->pVertexDisplayCommandBuffers[pGlobal->currentFrame], //
      pWindow->pSwapchainFramebuffers[imageIndex],                  //
      vertexBufferCount,                                            //
      pVertexBuffers,                                               //
      pVertexCounts,                                                //
      pGlobal->renderPass,                                          //
      pGlobal->graphicsPipelineLayout,                              //
      pWindow->graphicsPipeline,                                    //
      pWindow->swapchainExtent,                                     //
      mvp,                                                          //
      pGlobal->graphicsDescriptorSet,                               //
      (VkClearColorValue){.float32 = {0, 0, 0, 0}}                  //
  );

  free(pVertexBuffers);
  free(pVertexCounts);

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
  worldgen_state *pWg = new_worldgen_state(42);
  WorldState ws;
  wld_new_WorldState(       //
      &ws,                  //
      (ivec3){0, 0, 0},     //
      pWg,                  //
      global.graphicsQueue, //
      global.commandPool,   //
      global.device,        //
      global.physicalDevice //
  );

  // Set extent (window width and height)
  VkExtent2D swapchainExtent;
  getExtentWindow(&swapchainExtent, global.pWindow);

  // create camera
  vec3 loc = {0.0f, 0.0f, 0.0f};
  Camera camera = new_Camera(loc, swapchainExtent);

  // create window graphics
  AppGraphicsWindowState window;
  new_AppGraphicsWindowState(&window, &global, swapchainExtent);

  uint32_t fpsFrameCounter = 0;
  double fpsStartTime = glfwGetTime();

  uint32_t frameCounter = 0;

  // wait till close
  while (!glfwWindowShouldClose(global.pWindow)) {
    // glfw check for new events
    glfwPollEvents();

    // update camera
    updateCamera(&camera, global.pWindow);

    // update world
    wld_update(&ws);

    // project camera
    ivec3 highlightedIBlockCoords;
    BlockFaceKind highlightedFace;

    vec3 dir;
    const vec3 zero = {0.0f, 0.0f, 0.0f};
    vec3_sub(dir, zero, camera.basis.front);
    // attempt to get the highlighted face (if any);
    bool faceIsHighlighted = wld_trace_to_solid(
        highlightedIBlockCoords, &highlightedFace, camera.pos, dir, 800, &ws);
    if (faceIsHighlighted) {
      wld_highlight_face(highlightedIBlockCoords, highlightedFace, &ws);
      if(glfwGetMouseButton(global.pWindow, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        wld_set_block_at(0, &ws, highlightedIBlockCoords);
      } else if(glfwGetMouseButton(global.pWindow, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        ivec3 toPlaceIBlockCoords;
        wu_getAdjacentBlock(toPlaceIBlockCoords, highlightedIBlockCoords, highlightedFace);
        wld_set_block_at(1, &ws, toPlaceIBlockCoords);
      }
    } else {
      wld_clear_highlight_face(&ws);
    }

    // check camera chunk location,
    ivec3 camChunkCoord;
    blockCoords_to_worldChunkCoords(camChunkCoord, camera.pos);

    // if we have a new chunk location, set new chunk center
    if (!ivec3_eq(ws.centerLoc, camChunkCoord)) {
      wld_set_center(&ws, camChunkCoord);
    }

    // draw frame
    drawAppFrame(&window, &global, &camera, &ws);

    frameCounter++;

    if (frameCounter % 100 == 0) {
      vkDeviceWaitIdle(global.device);
      wld_clearGarbage(&ws);
    }

    fpsFrameCounter++;
    if (fpsFrameCounter >= 100) {
      double fpsEndTime = glfwGetTime();
      double fps = fpsFrameCounter / (fpsEndTime - fpsStartTime);
      LOG_ERROR_ARGS(ERR_LEVEL_INFO, "fps: %f", fps);
      fpsFrameCounter = 0;
      fpsStartTime = fpsEndTime;
    }
  }

  // wait for finish
  vkDeviceWaitIdle(global.device);

  // delete our world generator
  wld_delete_WorldState(&ws);
  delete_worldgen_state(pWg);

  // delete graphics resources
  delete_AppGraphicsWindowState(&window, &global);
  delete_GraphicsGlobalState(&global);

  return (EXIT_SUCCESS);
}
