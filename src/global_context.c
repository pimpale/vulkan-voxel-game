#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <vulkan/vulkan.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "constants.h"
#include "errors.h"

#include "global_context.h"

static VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT messageType,
              const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
              void *pUserData) {
  UNUSED(messageType);
  UNUSED(pUserData);

  /* set severity */
  ErrSeverity errSeverity = ERR_LEVEL_UNKNOWN;
  switch (messageSeverity) {
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
    errSeverity = ERR_LEVEL_INFO;
    break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
    errSeverity = ERR_LEVEL_INFO;
    break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
    errSeverity = ERR_LEVEL_WARN;
    break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
    errSeverity = ERR_LEVEL_ERROR;
    break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT:
    errSeverity = ERR_LEVEL_UNKNOWN;
    break;
  }
  /* log error */
  LOG_ERROR_ARGS(errSeverity, "vulkan validation layer: %s",
                 pCallbackData->pMessage);
  return (VK_FALSE);
}

/**
 * Requires the debug utils extension
 *
 * Creates a new debug callback that prints validation layer errors to stdout or
 * stderr, depending on their severity
 */
ErrVal new_DebugCallback(VkDebugUtilsMessengerEXT *pCallback,
                         const VkInstance instance) {
  VkDebugUtilsMessengerCreateInfoEXT createInfo = {0};
  createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  createInfo.pfnUserCallback = debugCallback;

  /* Returns a function pointer */
  PFN_vkCreateDebugUtilsMessengerEXT func =
      (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
          instance, "vkCreateDebugUtilsMessengerEXT");
  if (!func) {
    LOG_ERROR(ERR_LEVEL_FATAL, "Failed to find extension function");
    PANIC();
  }
  VkResult result = func(instance, &createInfo, NULL, pCallback);
  if (result != VK_SUCCESS) {
    LOG_ERROR_ARGS(ERR_LEVEL_FATAL,
                   "Failed to create debug callback, error code: %s",
                   vkstrerror(result));
    PANIC();
  }
  return (ERR_NOTSUPPORTED);
}

/**
 * Requires the debug utils extension
 * Deletes callback created in new_DebugCallback
 */
void delete_DebugCallback(VkDebugUtilsMessengerEXT *pCallback,
                          const VkInstance instance) {
  PFN_vkDestroyDebugUtilsMessengerEXT func =
      (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
          instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func != NULL) {
    func(instance, *pCallback, NULL);
  }
}

/* Get required extensions for a VkInstance */
static ErrVal new_RequiredInstanceExtensions(uint32_t *pEnabledExtensionCount,
                                             char ***pppEnabledExtensionNames) {
  /* define our own extensions */
  /* get GLFW extensions to use */
  uint32_t glfwExtensionCount = 0;
  const char **ppGlfwExtensionNames =
      glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
  *pEnabledExtensionCount = 1 + glfwExtensionCount;
  *pppEnabledExtensionNames =
      (char **)malloc(sizeof(char *) * (*pEnabledExtensionCount));

  if (!(*pppEnabledExtensionNames)) {
    LOG_ERROR_ARGS(ERR_LEVEL_FATAL, "failed to get required extensions: %s",
                   strerror(errno));
    PANIC();
  }

  /* Allocate buffers for extensions */
  for (uint32_t i = 0; i < *pEnabledExtensionCount; i++) {
    (*pppEnabledExtensionNames)[i] = malloc(VK_MAX_EXTENSION_NAME_SIZE);
  }
  /* Copy our extensions in  (we're malloccing everything to make it
   * simple to deallocate at the end without worrying about what needs to
   * be freed or not) */
  strncpy((*pppEnabledExtensionNames)[0], VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
          VK_MAX_EXTENSION_NAME_SIZE);
  for (uint32_t i = 0; i < glfwExtensionCount; i++) {
    strncpy((*pppEnabledExtensionNames)[i + 1], ppGlfwExtensionNames[i],
            VK_MAX_EXTENSION_NAME_SIZE);
  }
  return (ERR_OK);
}

/* Deletes char arrays allocated in new_RequiredInstanceExtensions */
static void delete_RequiredInstanceExtensions(uint32_t *pEnabledCount,
                                     char ***pppEnabledNames) {
  for (uint32_t i = 0; i < *pEnabledCount; i++) {
    free((*pppEnabledNames)[i]);
  }
  free(*pppEnabledNames);
}

static ErrVal new_ValidationLayers(uint32_t *pLayerCount,
                                   char ***pppLayerNames) {
  *pLayerCount = 1;
  *pppLayerNames = malloc(sizeof(char **) * (*pLayerCount));
  for (int i = 0; i < *pLayerCount; i++) {
    (*pppLayerNames)[i] = malloc(VK_MAX_EXTENSION_NAME_SIZE);
  }
  strncpy((*pppLayerNames)[0], "VK_LAYER_LUNARG_standard_validation",
          VK_MAX_EXTENSION_NAME_SIZE);
  return (ERR_OK);
}

/* Delete validation layer names allocated in new_ValidationLayers */
static void delete_ValidationLayers(uint32_t *pLayerCount,
                                    char ***pppLayerNames) {
  UNUSED(pLayerCount);
  for (int i = 0; i < *pLayerCount; i++) {
    free((*pppLayerNames)[i]);
  }
  free(*pppLayerNames);
  return;
}

/* Creates new VkInstance with sensible defaults */
static ErrVal new_Instance(VkInstance *pInstance, const uint32_t extensionCount,
                           const char *const *ppExtensionNames,
                           const uint32_t layerCount,
                           const char *const *ppLayerNames) {
  /* Create app info */
  VkApplicationInfo appInfo = {0};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = APPNAME;
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "None";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_0;

  /* Create info */
  VkInstanceCreateInfo createInfo = {0};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;
  createInfo.enabledExtensionCount = extensionCount;
  createInfo.ppEnabledExtensionNames = ppExtensionNames;
  createInfo.enabledLayerCount = layerCount;
  createInfo.ppEnabledLayerNames = ppLayerNames;
  /* Actually create instance */
  VkResult result = vkCreateInstance(&createInfo, NULL, pInstance);
  if (result != VK_SUCCESS) {
    LOG_ERROR_ARGS(ERR_LEVEL_FATAL, "Failed to create instance, error code: %s",
                   vkstrerror(result));
    PANIC();
  }
  return (ERR_OK);
}

/* Destroys instance created in new_Instance */
void delete_Instance(VkInstance *pInstance) {
  vkDestroyInstance(*pInstance, NULL);
  *pInstance = VK_NULL_HANDLE;
}

static ErrVal getPhysicalDevices(CgeGlobalContext *globalContext) {
  VkResult res = vkEnumeratePhysicalDevices(globalContext->instance, 
      &(globalContext->physicalDeviceCount), NULL);
  if (res != VK_SUCCESS || globalContext->physicalDeviceCount == 0) {
    LOG_ERROR(ERR_LEVEL_WARN, "no Vulkan capable device found");
    return (ERR_NOTSUPPORTED);
  }

  globalContext->pPhysicalDevices = malloc(globalContext->physicalDeviceCount *
      sizeof(VkPhysicalDevice));
  if (!globalContext->pPhysicalDevices) {
    LOG_ERROR_ARGS(ERR_LEVEL_FATAL, "failed to get physical device: %s",
                   strerror(errno));
    PANIC();
  }
  vkEnumeratePhysicalDevices(globalContext->instance,
      &(globalContext->physicalDeviceCount),
      globalContext->pPhysicalDevices);
}

ErrVal selectBestGraphicsDevices(CgeGlobalContext *globalContext,
                                 VkPhysicalDevice *pPhysicalDevices,
                                 uint32_t *physicalDeviceCount) {

  VkPhysicalDeviceProperties deviceProperties;
  VkPhysicalDevice selectedDevice = VK_NULL_HANDLE;
  for (uint32_t i = 0; i < globalContext->physicalDeviceCount; i++) {
    /* confirm it has required properties */
    vkGetPhysicalDeviceProperties(globalContext->pPhysicalDevices[i], &deviceProperties);
    uint32_t deviceQueueIndex;
    uint32_t ret =
        getDeviceQueueIndex(&deviceQueueIndex, arr[i],
                            VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);
    if (ret == VK_SUCCESS) {
      selectedDevice = arr[i];
      break;
    }
  }
  free(arr);
  if (selectedDevice == VK_NULL_HANDLE) {
    LOG_ERROR(ERR_LEVEL_WARN, "no suitable Vulkan device found");
    return (ERR_NOTSUPPORTED);
  } else {
    *pDevice = selectedDevice;
    return (ERR_OK);
  }
}

ErrVal selectBestComputeDevices(CgeGlobalContext *globalContext,
                                VkPhysicalDevice *pPhysicalDevices,
                                uint32_t *physicalDeviceCount);

ErrVal new_CgeGlobalContext(CgeGlobalContext *globalContext) {

  uint32_t extensionCount;
  char **ppExtensionNames;

  uint32_t layerCount;
  char **ppLayerNames;

  /* Extensions, Layers, and Device Extensions declared */
  new_RequiredInstanceExtensions(&extensionCount, &ppExtensionNames);
  new_ValidationLayers(&layerCount, &ppLayerNames);

  /* Create instance */
  new_Instance(&(globalContext->instance), extensionCount,
               (const char *const *)ppExtensionNames, layerCount,
               (const char *const *)ppLayerNames);
  new_DebugCallback(&(globalContext->callback), globalContext->instance);

  delete_RequiredInstanceExtensions(&extensionCount, &ppExtensionNames);
  delete_ValidationLayers(&layerCount, &ppLayerNames);
  return (ERR_OK);
}

void delete_CgeGlobalContext(CgeGlobalContext *globalContext) {
  delete_DebugCallback(&(globalContext->callback), globalContext->instance);
  delete_Instance(&(globalContext->instance));
}



