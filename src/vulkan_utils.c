#include "vulkan_utils.h"

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <vulkan/vulkan.h>

#include "linmath.h"

#include "constants.h"
#include "errors.h"
#include "utils.h"

char *vkstrerror(VkResult err) {
  switch (err) {
  case VK_SUCCESS:
    return ("VK_SUCCESS");
  case VK_NOT_READY:
    return ("VK_NOT_READY");
  case VK_TIMEOUT:
    return ("VK_TIMEOUT");
  case VK_EVENT_SET:
    return ("VK_EVENT_SET");
  case VK_EVENT_RESET:
    return ("VK_EVENT_RESET");
  case VK_INCOMPLETE:
    return ("VK_INCOMPLETE");
  case VK_ERROR_OUT_OF_HOST_MEMORY:
    return ("VK_ERROR_OUT_OF_HOST_MEMORY");
  case VK_ERROR_OUT_OF_DEVICE_MEMORY:
    return ("VK_ERROR_OUT_OF_DEVICE_MEMORY");
  case VK_ERROR_INITIALIZATION_FAILED:
    return ("VK_ERROR_INITIALIZATION_FAILED");
  case VK_ERROR_DEVICE_LOST:
    return ("VK_ERROR_DEVICE_LOST");
  case VK_ERROR_MEMORY_MAP_FAILED:
    return ("VK_ERROR_MEMORY_MAP_FAILED");
  case VK_ERROR_LAYER_NOT_PRESENT:
    return ("VK_ERROR_LAYER_NOT_PRESENT");
  case VK_ERROR_EXTENSION_NOT_PRESENT:
    return ("VK_ERROR_EXTENSION_NOT_PRESENT");
  case VK_ERROR_FEATURE_NOT_PRESENT:
    return ("VK_ERROR_FEATURE_NOT_PRESENT");
  case VK_ERROR_INCOMPATIBLE_DRIVER:
    return ("VK_ERROR_INCOMPATIBLE_DRIVER");
  case VK_ERROR_TOO_MANY_OBJECTS:
    return ("VK_ERROR_TOO_MANY_OBJECTS");
  case VK_ERROR_FORMAT_NOT_SUPPORTED:
    return ("VK_ERROR_FORMAT_NOT_SUPPORTED");
  case VK_ERROR_FRAGMENTED_POOL:
    return ("VK_ERROR_FRAGMENTED_POOL");
  case VK_ERROR_OUT_OF_POOL_MEMORY:
    return ("VK_ERROR_OUT_OF_POOL_MEMORY");
  case VK_ERROR_INVALID_EXTERNAL_HANDLE:
    return ("VK_ERROR_INVALID_EXTERNAL_HANDLE");
  case VK_ERROR_SURFACE_LOST_KHR:
    return ("VK_ERROR_SURFACE_LOST_KHR");
  case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
    return ("VK_ERROR_NATIVE_WINDOW_IN_USE_KHR");
  case VK_SUBOPTIMAL_KHR:
    return ("VK_SUBOPTIMAL_KHR");
  case VK_ERROR_OUT_OF_DATE_KHR:
    return ("VK_ERROR_OUT_OF_DATE_KHR");
  case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
    return ("VK_ERROR_INCOMPATIBLE_DISPLAY_KHR");
  case VK_ERROR_VALIDATION_FAILED_EXT:
    return ("VK_ERROR_VALIDATION_FAILED_EXT");
  case VK_ERROR_INVALID_SHADER_NV:
    return ("VK_ERROR_INVALID_SHADER_NV");
  case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:
    return ("VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_"
            "EXT");
  case VK_ERROR_FRAGMENTATION_EXT:
    return ("VK_ERROR_FRAGMENTATION_EXT");
  case VK_ERROR_NOT_PERMITTED_EXT:
    return ("VK_ERROR_NOT_PERMITTED_EXT");
  default:
    return ("UNKNOWN_ERROR");
  }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT messageType,
              const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
              void *pUserData) {
  UNUSED(messageType);
  UNUSED(pUserData);

  /* set severity */
  uint32_t errSeverity;
  switch (messageSeverity) {
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
    errSeverity = INFO;
    break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
    errSeverity = INFO;
    break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
    errSeverity = WARN;
    break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
    errSeverity = ERROR;
    break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT:
    errSeverity = UNKNOWN;
    break;
  default:
    errSeverity = UNKNOWN;
    break;
  }
  /* log error */
  errLog(errSeverity, "vulkan validation layer: %s", pCallbackData->pMessage);
  return (VK_FALSE);
}

/* Get required extensions for a VkInstance */
ErrVal new_RequiredInstanceExtensions(uint32_t *pEnabledExtensionCount,
                                      char ***pppEnabledExtensionNames) {
  /* define our own extensions */
  /* get GLFW extensions to use */
  uint32_t glfwExtensionCount = 0;
  const char **ppGlfwExtensionNames =
      glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
  *pEnabledExtensionCount = 1 + glfwExtensionCount;
  *pppEnabledExtensionNames =
      malloc(sizeof(char *) * (*pEnabledExtensionCount));

  if (!(*pppEnabledExtensionNames)) {
    errLog(FATAL, "failed to get required extensions: %s", strerror(errno));
    panic();
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
void delete_RequiredInstanceExtensions(uint32_t *pEnabledExtensionCount,
                                       char ***pppEnabledExtensionNames) {
  for (uint32_t i = 0; i < *pEnabledExtensionCount; i++) {
    free((*pppEnabledExtensionNames)[i]);
  }
  free(*pppEnabledExtensionNames);
}

/* Get required layer names for validation layers */
ErrVal new_ValidationLayers(uint32_t *pLayerCount, char ***pppLayerNames) {
  *pLayerCount = 1;
  *pppLayerNames = malloc(sizeof(char *) * sizeof(*pLayerCount));
  **pppLayerNames = "VK_LAYER_LUNARG_standard_validation";
  return (ERR_OK);
}

/* Delete validation layer names allocated in new_ValidationLayers */
void delete_ValidationLayers(uint32_t *pLayerCount, char ***pppLayerNames) {
  UNUSED(pLayerCount);
  free(*pppLayerNames);
}

/* Get array of required device extensions for running graphics (swapchain) */
ErrVal new_RequiredDeviceExtensions(uint32_t *pEnabledExtensionCount,
                                    char ***pppEnabledExtensionNames) {
  *pEnabledExtensionCount = 1;
  *pppEnabledExtensionNames =
      malloc(sizeof(char *) * sizeof(*pEnabledExtensionCount));
  **pppEnabledExtensionNames = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
  return (ERR_OK);
}

/* Delete array allocated in new_RequiredDeviceExtensions */
void delete_RequiredDeviceExtensions(uint32_t *pEnabledExtensionCount,
                                     char ***pppEnabledExtensionNames) {
  UNUSED(pEnabledExtensionCount);
  free(*pppEnabledExtensionNames);
}

/* Creates new VkInstance with sensible defaults */
ErrVal new_Instance(VkInstance *pInstance, const uint32_t enabledExtensionCount,
                    const char *const *ppEnabledExtensionNames,
                    const uint32_t enabledLayerCount,
                    const char *const *ppEnabledLayerNames) {
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
  createInfo.enabledExtensionCount = enabledExtensionCount;
  createInfo.ppEnabledExtensionNames = ppEnabledExtensionNames;
  createInfo.enabledLayerCount = enabledLayerCount;
  createInfo.ppEnabledLayerNames = ppEnabledLayerNames;
  /* Actually create instance */
  VkResult result = vkCreateInstance(&createInfo, NULL, pInstance);
  if (result != VK_SUCCESS) {
    errLog(FATAL, "Failed to create instance, error code: %s",
           vkstrerror(result));
    panic();
  }
  return (ERR_OK);
}

/* Destroys instance created in new_Instance */
void delete_Instance(VkInstance *pInstance) {
  vkDestroyInstance(*pInstance, NULL);
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
    errLog(FATAL, "Failed to find extension function");
    panic();
  }
  VkResult result = func(instance, &createInfo, NULL, pCallback);
  if (result != VK_SUCCESS) {
    errLog(FATAL, "Failed to create debug callback, error code: %s",
           vkstrerror(result));
    panic();
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
/**
 * gets the best physical device, checks if it has all necessary capabilities.
 */
ErrVal getPhysicalDevice(VkPhysicalDevice *pDevice, const VkInstance instance) {
  uint32_t deviceCount = 0;
  VkResult res = vkEnumeratePhysicalDevices(instance, &deviceCount, NULL);
  if (res != VK_SUCCESS || deviceCount == 0) {
    errLog(WARN, "no Vulkan capable device found");
    return (ERR_NOTSUPPORTED);
  }
  VkPhysicalDevice *arr = malloc(deviceCount * sizeof(VkPhysicalDevice));
  if (!arr) {
    errLog(FATAL, "failed to allocate memory: %s", strerror(errno));
    panic();
  }
  vkEnumeratePhysicalDevices(instance, &deviceCount, arr);

  VkPhysicalDeviceProperties deviceProperties;
  VkPhysicalDevice selectedDevice = VK_NULL_HANDLE;
  for (uint32_t i = 0; i < deviceCount; i++) {
    /* TODO confirm it has required properties */
    vkGetPhysicalDeviceProperties(arr[i], &deviceProperties);
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
    errLog(WARN, "no suitable Vulkan device found");
    return (ERR_NOTSUPPORTED);
  } else {
    *pDevice = selectedDevice;
    return (ERR_OK);
  }
}

/**
 * Deletes VkDevice created in new_Device
 */
void delete_Device(VkDevice *pDevice) { vkDestroyDevice(*pDevice, NULL); }

/**
 * Sets deviceQueueIndex to queue family index corresponding to the bit passed
 * in for the device
 */
ErrVal getDeviceQueueIndex(uint32_t *deviceQueueIndex,
                           const VkPhysicalDevice device,
                           const VkQueueFlags bit) {
  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, NULL);
  if (queueFamilyCount == 0) {
    errLog(WARN, "no device queues found");
    return (ERR_NOTSUPPORTED);
  }
  VkQueueFamilyProperties *arr =
      malloc(queueFamilyCount * sizeof(VkQueueFamilyProperties));
  if (!arr) {
    errLog(FATAL, "Failed to get device queue index: %s", strerror(errno));
    panic();
  }
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, arr);
  for (uint32_t i = 0; i < queueFamilyCount; i++) {
    if (arr[i].queueCount > 0 && (arr[0].queueFlags & bit)) {
      free(arr);
      *deviceQueueIndex = i;
      return (ERR_OK);
    }
  }
  free(arr);
  errLog(WARN, "no suitable device queue found");
  return (ERR_NOTSUPPORTED);
}

ErrVal getPresentQueueIndex(uint32_t *pPresentQueueIndex,
                            const VkPhysicalDevice physicalDevice,
                            const VkSurfaceKHR surface) {
  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount,
                                           NULL);
  if (queueFamilyCount == 0) {
    errLog(WARN, "no queues found");
    return (ERR_NOTSUPPORTED);
  }
  VkQueueFamilyProperties *arr =
      malloc(queueFamilyCount * sizeof(VkQueueFamilyProperties));
  if (!arr) {
    errLog(FATAL, "Failed to get present queue index: %s", strerror(errno));
    panic();
  }
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount,
                                           arr);
  for (uint32_t i = 0; i < queueFamilyCount; i++) {
    VkBool32 surfaceSupport;
    VkResult res = vkGetPhysicalDeviceSurfaceSupportKHR(
        physicalDevice, i, surface, &surfaceSupport);
    if (res == VK_SUCCESS && surfaceSupport) {
      *pPresentQueueIndex = i;
      free(arr);
      return (ERR_OK);
    }
  }
  free(arr);
  return (ERR_NOTSUPPORTED);
}

ErrVal new_Device(VkDevice *pDevice, const VkPhysicalDevice physicalDevice,
                  const uint32_t deviceQueueIndex,
                  const uint32_t enabledExtensionCount,
                  const char *const *ppEnabledExtensionNames,
                  const uint32_t enabledLayerCount,
                  const char *const *ppEnabledLayerNames) {
  VkPhysicalDeviceFeatures deviceFeatures = {0};
  VkDeviceQueueCreateInfo queueCreateInfo = {0};
  queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queueCreateInfo.queueFamilyIndex = deviceQueueIndex;
  queueCreateInfo.queueCount = 1;
  float queuePriority = 1.0f;
  queueCreateInfo.pQueuePriorities = &queuePriority;

  VkDeviceCreateInfo createInfo = {0};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  createInfo.pQueueCreateInfos = &queueCreateInfo;
  createInfo.queueCreateInfoCount = 1;
  createInfo.pEnabledFeatures = &deviceFeatures;
  createInfo.enabledExtensionCount = enabledExtensionCount;
  createInfo.ppEnabledExtensionNames = ppEnabledExtensionNames;
  createInfo.enabledLayerCount = enabledLayerCount;
  createInfo.ppEnabledLayerNames = ppEnabledLayerNames;

  VkResult res = vkCreateDevice(physicalDevice, &createInfo, NULL, pDevice);
  if (res != VK_SUCCESS) {
    errLog(ERROR, "Failed to create device, error code: %s", vkstrerror(res));
    panic();
  }
  return (ERR_OK);
}

ErrVal getQueue(VkQueue *pQueue, const VkDevice device,
                const uint32_t deviceQueueIndex) {
  vkGetDeviceQueue(device, deviceQueueIndex, 0, pQueue);
  return (ERR_OK);
}

ErrVal new_SwapChain(VkSwapchainKHR *pSwapChain, uint32_t *pSwapChainImageCount,
                     const VkSwapchainKHR oldSwapChain,
                     const VkSurfaceFormatKHR surfaceFormat,
                     const VkPhysicalDevice physicalDevice,
                     const VkDevice device, const VkSurfaceKHR surface,
                     const VkExtent2D extent, const uint32_t graphicsIndex,
                     const uint32_t presentIndex) {
  VkSurfaceCapabilitiesKHR capabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface,
                                            &capabilities);

  *pSwapChainImageCount = capabilities.minImageCount + 1;
  if (capabilities.maxImageCount > 0 &&
      *pSwapChainImageCount > capabilities.maxImageCount) {
    *pSwapChainImageCount = capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR createInfo = {0};
  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface = surface;
  createInfo.minImageCount = *pSwapChainImageCount;
  createInfo.imageFormat = surfaceFormat.format;
  createInfo.imageColorSpace = surfaceFormat.colorSpace;
  createInfo.imageExtent = extent;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  uint32_t queueFamilyIndices[] = {graphicsIndex, presentIndex};
  if (graphicsIndex != presentIndex) {
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = queueFamilyIndices;
  } else {
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;  /* Optional */
    createInfo.pQueueFamilyIndices = NULL; /* Optional */
  }

  createInfo.preTransform = capabilities.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  /* guaranteed to be available */
  createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
  createInfo.clipped = VK_TRUE;
  createInfo.oldSwapchain = oldSwapChain;
  VkResult res = vkCreateSwapchainKHR(device, &createInfo, NULL, pSwapChain);
  if (res != VK_SUCCESS) {
    errLog(ERROR, "Failed to create swap chain, error code: %s",
           vkstrerror(res));
    panic();
  }
  return (ERR_OK);
}

void delete_SwapChain(VkSwapchainKHR *pSwapChain, const VkDevice device) {
  vkDestroySwapchainKHR(device, *pSwapChain, NULL);
}

ErrVal getPreferredSurfaceFormat(VkSurfaceFormatKHR *pSurfaceFormat,
                                 const VkPhysicalDevice physicalDevice,
                                 const VkSurfaceKHR surface) {
  uint32_t formatCount = 0;
  VkSurfaceFormatKHR *pSurfaceFormats;
  vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount,
                                       NULL);
  if (formatCount != 0) {
    pSurfaceFormats = malloc(formatCount * sizeof(VkSurfaceFormatKHR));
    if (!pSurfaceFormats) {
      errLog(FATAL, "could not get preferred format: %s", strerror(errno));
      panic();
    }
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount,
                                         pSurfaceFormats);
  } else {
    pSurfaceFormats = NULL;
    return (ERR_NOTSUPPORTED);
  }

  VkSurfaceFormatKHR preferredFormat = {0};
  if (formatCount == 1 && pSurfaceFormats[0].format == VK_FORMAT_UNDEFINED) {
    /* If it has no preference, use our own */
    preferredFormat = pSurfaceFormats[0];
  } else if (formatCount != 0) {
    /* we default to the first one in the list */
    preferredFormat = pSurfaceFormats[0];
    /* However,  we check to make sure that what we want is in there
     */
    for (uint32_t i = 0; i < formatCount; i++) {
      VkSurfaceFormatKHR availableFormat = pSurfaceFormats[i];
      if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
          availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
        preferredFormat = availableFormat;
      }
    }
  } else {
    errLog(ERROR, "no formats available");
    free(pSurfaceFormats);
    return (ERR_NOTSUPPORTED);
  }

  free(pSurfaceFormats);

  *pSurfaceFormat = preferredFormat;
  return (VK_SUCCESS);
}

ErrVal new_SwapChainImages(VkImage **ppSwapChainImages, uint32_t *pImageCount,
                           const VkDevice device,
                           const VkSwapchainKHR swapChain) {
  vkGetSwapchainImagesKHR(device, swapChain, pImageCount, NULL);

  if (pImageCount == 0) {
    errLog(WARN, "cannot create zero images");
    return (ERR_UNSAFE);
  }

  *ppSwapChainImages = malloc((*pImageCount) * sizeof(VkImage));
  if (!*ppSwapChainImages) {
    errLog(FATAL, "failed to get swap chain images: %s", strerror(errno));
    panic();
  }
  VkResult res = vkGetSwapchainImagesKHR(device, swapChain, pImageCount,
                                         *ppSwapChainImages);
  if (res != VK_SUCCESS) {
    errLog(WARN, "failed to get swap chain images, error: %s", vkstrerror(res));
    return (ERR_UNKNOWN);
  } else {
    return (ERR_OK);
  }
}

void delete_SwapChainImages(VkImage **ppImages) {
  free(*ppImages);
  *ppImages = NULL;
}

ErrVal new_ImageView(VkImageView *pImageView, const VkDevice device,
                     const VkImage image, const VkFormat format) {
  VkImageViewCreateInfo createInfo = {0};
  createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  createInfo.image = image;
  createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  createInfo.format = format;
  createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
  createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
  createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
  createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  createInfo.subresourceRange.baseMipLevel = 0;
  createInfo.subresourceRange.levelCount = 1;
  createInfo.subresourceRange.baseArrayLayer = 0;
  createInfo.subresourceRange.layerCount = 1;
  VkResult ret = vkCreateImageView(device, &createInfo, NULL, pImageView);
  if (ret != VK_SUCCESS) {
    errLog(FATAL, "could not create image view, error code: %s",
           vkstrerror(ret));
    panic();
  }
  return (ERR_OK);
}

void delete_ImageView(VkImageView *pImageView, VkDevice device) {
  vkDestroyImageView(device, *pImageView, NULL);
}

ErrVal new_SwapChainImageViews(VkImageView **ppImageViews,
                               const VkDevice device, const VkFormat format,
                               const uint32_t imageCount,
                               const VkImage *pSwapChainImages) {
  if (imageCount == 0) {
    errLog(WARN, "cannot create zero image views");
    return (ERR_BADARGS);
  }
  VkImageView *pImageViews = malloc(imageCount * sizeof(VkImageView));
  if (!pImageViews) {
    errLog(FATAL, "could not create swap chain image views: %s",
           strerror(errno));
    panic();
  }

  for (uint32_t i = 0; i < imageCount; i++) {
    uint32_t ret =
        new_ImageView(&(pImageViews[i]), device, pSwapChainImages[i], format);
    if (ret != VK_SUCCESS) {
      errLog(FATAL, "could not create image view, error code: %s",
             vkstrerror(ret));
      panic();
    }
  }

  *ppImageViews = pImageViews;
  return (ERR_OK);
}

void delete_SwapChainImageViews(VkImageView **ppImageViews, uint32_t imageCount,
                                const VkDevice device) {
  for (uint32_t i = 0; i < imageCount; i++) {
    delete_ImageView(&((*ppImageViews)[i]), device);
  }
  free(*ppImageViews);
  *ppImageViews = NULL;
}

ErrVal new_ShaderModule(VkShaderModule *pShaderModule, const VkDevice device,
                        const uint32_t codeSize, const uint32_t *pCode) {
  VkShaderModuleCreateInfo createInfo = {0};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = codeSize;
  createInfo.pCode = pCode;
  VkResult res = vkCreateShaderModule(device, &createInfo, NULL, pShaderModule);
  if (res != VK_SUCCESS) {
    errLog(FATAL, "failed to create shader module");
  }
  return (ERR_OK);
}

ErrVal new_ShaderModuleFromFile(VkShaderModule *pShaderModule,
                                const VkDevice device, char *filename) {
  uint32_t *shaderFileContents;
  uint32_t shaderFileLength;
  readShaderFile(filename, &shaderFileLength, &shaderFileContents);
  uint32_t retval = new_ShaderModule(pShaderModule, device, shaderFileLength,
                                     shaderFileContents);
  free(shaderFileContents);
  return (retval);
}

void delete_ShaderModule(VkShaderModule *pShaderModule, const VkDevice device) {
  vkDestroyShaderModule(device, *pShaderModule, NULL);
}

ErrVal new_RenderPass(VkRenderPass *pRenderPass, const VkDevice device,
                      const VkFormat swapChainImageFormat) {
  VkAttachmentDescription colorAttachment = {0};
  colorAttachment.format = swapChainImageFormat;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colorAttachmentRef = {0};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass = {0};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;

  VkRenderPassCreateInfo renderPassInfo = {0};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = 1;
  renderPassInfo.pAttachments = &colorAttachment;
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;

  VkSubpassDependency dependency = {0};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                             VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &dependency;

  VkResult res = vkCreateRenderPass(device, &renderPassInfo, NULL, pRenderPass);
  if (res != VK_SUCCESS) {
    errLog(FATAL, "Could not create render pass, error: %s", vkstrerror(res));
    panic();
  }
  return (ERR_OK);
}

void delete_RenderPass(VkRenderPass *pRenderPass, const VkDevice device) {
  vkDestroyRenderPass(device, *pRenderPass, NULL);
}

ErrVal new_VertexDisplayPipelineLayout(VkPipelineLayout *pPipelineLayout,
                                       const VkDevice device) {
  VkPushConstantRange pushConstantRange = {0};
  pushConstantRange.offset = 0;
  pushConstantRange.size = sizeof(mat4x4);
  pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {0};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 0;
  pipelineLayoutInfo.pushConstantRangeCount = 1;
  pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
  VkResult res = vkCreatePipelineLayout(device, &pipelineLayoutInfo, NULL,
                                        pPipelineLayout);
  if (res != VK_SUCCESS) {
    errLog(FATAL, "failed to create pipeline layout with error: %s",
           vkstrerror(res));
    panic();
  }
  return (ERR_OK);
}

void delete_PipelineLayout(VkPipelineLayout *pPipelineLayout,
                           const VkDevice device) {
  vkDestroyPipelineLayout(device, *pPipelineLayout, NULL);
}

ErrVal new_VertexDisplayPipeline(VkPipeline *pGraphicsPipeline,
                                 const VkDevice device,
                                 const VkShaderModule vertShaderModule,
                                 const VkShaderModule fragShaderModule,
                                 const VkExtent2D extent,
                                 const VkRenderPass renderPass,
                                 const VkPipelineLayout pipelineLayout) {
  VkPipelineShaderStageCreateInfo vertShaderStageInfo = {0};
  vertShaderStageInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = vertShaderModule;
  vertShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo fragShaderStageInfo = {0};
  fragShaderStageInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = fragShaderModule;
  fragShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo shaderStages[2] = {vertShaderStageInfo,
                                                     fragShaderStageInfo};

  VkVertexInputBindingDescription bindingDescription = {0};
  bindingDescription.binding = 0;
  bindingDescription.stride = sizeof(struct Vertex);
  bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  VkVertexInputAttributeDescription attributeDescriptions[2];

  attributeDescriptions[0].binding = 0;
  attributeDescriptions[0].location = 0;
  attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
  attributeDescriptions[0].offset = offsetof(struct Vertex, position);

  attributeDescriptions[1].binding = 0;
  attributeDescriptions[1].location = 1;
  attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
  attributeDescriptions[1].offset = offsetof(struct Vertex, color);

  VkPipelineVertexInputStateCreateInfo vertexInputInfo = {0};
  vertexInputInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount = 1;
  vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
  vertexInputInfo.vertexAttributeDescriptionCount = 2;
  vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions;

  VkPipelineInputAssemblyStateCreateInfo inputAssembly = {0};
  inputAssembly.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  VkViewport viewport = {0};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float)extent.width;
  viewport.height = (float)extent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor = {0};
  scissor.offset.x = 0;
  scissor.offset.y = 0;
  scissor.extent = extent;

  VkPipelineViewportStateCreateInfo viewportState = {0};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.pViewports = &viewport;
  viewportState.scissorCount = 1;
  viewportState.pScissors = &scissor;

  VkPipelineRasterizationStateCreateInfo rasterizer = {0};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0f;
  rasterizer.cullMode = VK_CULL_MODE_NONE; /* VK_CULL_MODE_BACK_BIT; */
  rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterizer.depthBiasEnable = VK_FALSE;

  VkPipelineMultisampleStateCreateInfo multisampling = {0};
  multisampling.sType =
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkPipelineColorBlendAttachmentState colorBlendAttachment = {0};
  colorBlendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_FALSE;

  VkPipelineColorBlendStateCreateInfo colorBlending = {0};
  colorBlending.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.logicOp = VK_LOGIC_OP_COPY;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;
  colorBlending.blendConstants[0] = 0.0f;
  colorBlending.blendConstants[1] = 0.0f;
  colorBlending.blendConstants[2] = 0.0f;
  colorBlending.blendConstants[3] = 0.0f;

  VkGraphicsPipelineCreateInfo pipelineInfo = {0};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = shaderStages;
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.layout = pipelineLayout;
  pipelineInfo.renderPass = renderPass;
  pipelineInfo.subpass = 0;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

  if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL,
                                pGraphicsPipeline) != VK_SUCCESS) {
    errLog(FATAL, "failed to create graphics pipeline!");
    panic();
  }
  return (ERR_OK);
}

void delete_Pipeline(VkPipeline *pPipeline, const VkDevice device) {
  vkDestroyPipeline(device, *pPipeline, NULL);
}

ErrVal new_Framebuffer(VkFramebuffer *pFramebuffer, const VkDevice device,
                       const VkRenderPass renderPass,
                       const VkImageView imageView,
                       const VkExtent2D swapChainExtent) {
  VkFramebufferCreateInfo framebufferInfo = {0};
  framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  framebufferInfo.renderPass = renderPass;
  framebufferInfo.attachmentCount = 1;
  framebufferInfo.pAttachments = (VkImageView[]){imageView};
  framebufferInfo.width = swapChainExtent.width;
  framebufferInfo.height = swapChainExtent.height;
  framebufferInfo.layers = 1;
  VkResult res =
      vkCreateFramebuffer(device, &framebufferInfo, NULL, pFramebuffer);
  if (res == VK_SUCCESS) {
    return (ERR_OK);
  } else {
    errLog(WARN, "failed to create framebuffers: %s", vkstrerror(res));
    return (ERR_UNKNOWN);
  }
}

void delete_Framebuffer(VkFramebuffer *pFramebuffer, VkDevice device) {
  vkDestroyFramebuffer(device, *pFramebuffer, NULL);
  *pFramebuffer = VK_NULL_HANDLE;
}

ErrVal new_SwapChainFramebuffers(VkFramebuffer **ppFramebuffers,
                                 const VkDevice device,
                                 const VkRenderPass renderPass,
                                 const VkExtent2D swapChainExtent,
                                 const uint32_t imageCount,
                                 const VkImageView *pSwapChainImageViews) {
  VkFramebuffer *tmp = malloc(imageCount * sizeof(VkFramebuffer));
  if (!tmp) {
    errLog(FATAL, "could not create framebuffers: %s", strerror(errno));
    panic();
  }

  for (uint32_t i = 0; i < imageCount; i++) {
    uint32_t res = new_Framebuffer(&(tmp[i]), device, renderPass,
                                   pSwapChainImageViews[i], swapChainExtent);
    if (res != VK_SUCCESS) {
      errLog(ERROR, "could not create framebuffer, error code: %s",
             vkstrerror(res));
      free(tmp);
      return (res);
    }
  }

  *ppFramebuffers = tmp;
  return (ERR_OK);
}

void delete_SwapChainFramebuffers(VkFramebuffer **ppFramebuffers,
                                  const uint32_t imageCount,
                                  const VkDevice device) {
  for (uint32_t i = 0; i < imageCount; i++) {
    delete_Framebuffer(&((*ppFramebuffers)[i]), device);
  }
  free(*ppFramebuffers);
  *ppFramebuffers = NULL;
}

ErrVal new_CommandPool(VkCommandPool *pCommandPool, const VkDevice device,
                       const uint32_t queueFamilyIndex) {
  VkCommandPoolCreateInfo poolInfo = {0};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.queueFamilyIndex = queueFamilyIndex;
  poolInfo.flags = 0;
  VkResult ret = vkCreateCommandPool(device, &poolInfo, NULL, pCommandPool);
  return (ret);
}

void delete_CommandPool(VkCommandPool *pCommandPool, const VkDevice device) {
  vkDestroyCommandPool(device, *pCommandPool, NULL);
}

ErrVal new_VertexDisplayCommandBuffers(
    VkCommandBuffer **ppCommandBuffers, const VkBuffer vertexBuffer,
    const uint32_t vertexCount, const VkDevice device,
    const VkRenderPass renderPass,
    const VkPipelineLayout vertexDisplayPipelineLayout,
    const VkPipeline vertexDisplayPipeline, const VkCommandPool commandPool,
    const VkExtent2D swapChainExtent, const uint32_t swapChainFramebufferCount,
    const VkFramebuffer *pSwapChainFramebuffers, const mat4x4 cameraTransform) {

  VkCommandBufferAllocateInfo allocInfo = {0};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = commandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = swapChainFramebufferCount;

  VkCommandBuffer *pCommandBuffers =
      malloc(swapChainFramebufferCount * sizeof(VkCommandBuffer));
  if (!pCommandBuffers) {
    errLog(FATAL, "Failed to create graphics command buffers: %s",
           strerror(errno));
    panic();
  }

  VkResult allocateCommandBuffersRetVal =
      vkAllocateCommandBuffers(device, &allocInfo, pCommandBuffers);
  if (allocateCommandBuffersRetVal != VK_SUCCESS) {
    errLog(FATAL, "Failed to create graphics command buffers, error code: %s",
           vkstrerror(allocateCommandBuffersRetVal));
    panic();
  }

  for (size_t i = 0; i < swapChainFramebufferCount; i++) {
    VkCommandBufferBeginInfo beginInfo = {0};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

    if (vkBeginCommandBuffer(pCommandBuffers[i], &beginInfo) != VK_SUCCESS) {
      errLog(FATAL, "Failed to record into graphics command buffer");
      panic();
    }

    VkRenderPassBeginInfo renderPassInfo = {0};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = pSwapChainFramebuffers[i];
    renderPassInfo.renderArea.offset = (VkOffset2D){0, 0};
    renderPassInfo.renderArea.extent = swapChainExtent;

    VkClearValue clearColor;
    clearColor.color.float32[0] = 0.0f;
    clearColor.color.float32[1] = 0.0f;
    clearColor.color.float32[2] = 0.0f;
    clearColor.color.float32[3] = 0.0f;

    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(pCommandBuffers[i], &renderPassInfo,
                         VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(pCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
                      vertexDisplayPipeline);

    vkCmdPushConstants(pCommandBuffers[i], vertexDisplayPipelineLayout,
                       VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat4x4),
                       cameraTransform);
    VkBuffer vertexBuffers[] = {vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(pCommandBuffers[i], 0, 1, vertexBuffers, offsets);

    vkCmdDraw(pCommandBuffers[i], vertexCount, 1, 0, 0);
    vkCmdEndRenderPass(pCommandBuffers[i]);

    VkResult endCommandBufferRetVal = vkEndCommandBuffer(pCommandBuffers[i]);
    if (endCommandBufferRetVal != VK_SUCCESS) {
      errLog(FATAL, "Failed to record command buffer, error code: %s",
             vkstrerror(endCommandBufferRetVal));
      panic();
    }
  }

  *ppCommandBuffers = pCommandBuffers;
  return (VK_SUCCESS);
}

void delete_CommandBuffers(VkCommandBuffer **ppCommandBuffers) {
  free(*ppCommandBuffers);
  *ppCommandBuffers = NULL;
}

ErrVal new_Semaphore(VkSemaphore *pSemaphore, const VkDevice device) {
  VkSemaphoreCreateInfo semaphoreInfo = {0};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  return (vkCreateSemaphore(device, &semaphoreInfo, NULL, pSemaphore));
}

void delete_Semaphore(VkSemaphore *pSemaphore, const VkDevice device) {
  vkDestroySemaphore(device, *pSemaphore, NULL);
  *pSemaphore = NULL;
}

ErrVal new_Semaphores(VkSemaphore **ppSemaphores, const uint32_t semaphoreCount,
                      const VkDevice device) {
  if (semaphoreCount == 0) {
    errLog(WARN, "failed to create semaphores: %s",
           "Failed to allocate 0 bytes of memory");
  }
  *ppSemaphores = malloc(semaphoreCount * sizeof(VkSemaphore));
  if (*ppSemaphores == NULL) {
    errLog(FATAL, "Failed to create semaphores: %s", strerror(errno));
  }

  for (uint32_t i = 0; i < semaphoreCount; i++) {
    new_Semaphore(&(*ppSemaphores)[i], device);
  }
  return (ERR_OK);
}

void delete_Semaphores(VkSemaphore **ppSemaphores,
                       const uint32_t semaphoreCount, const VkDevice device) {
  for (uint32_t i = 0; i < semaphoreCount; i++) {
    delete_Semaphore(&((*ppSemaphores)[i]), device);
  }
  free(*ppSemaphores);
  *ppSemaphores = NULL;
}

ErrVal new_Fence(VkFence *pFence, const VkDevice device) {
  VkFenceCreateInfo fenceInfo = {0};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  return (vkCreateFence(device, &fenceInfo, NULL, pFence));
}

void delete_Fence(VkFence *pFence, const VkDevice device) {
  vkDestroyFence(device, *pFence, NULL);
  *pFence = VK_NULL_HANDLE;
}

ErrVal new_Fences(VkFence **ppFences, const uint32_t fenceCount,
                  const VkDevice device) {
  if (fenceCount == 0) {
    errLog(WARN, "cannot allocate 0 bytes of memory");
    return (ERR_UNSAFE);
  }
  *ppFences = malloc(fenceCount * sizeof(VkDevice));
  if (!*ppFences) {
    errLog(FATAL, "failed to create memory fence; %s", strerror(errno));
    panic();
  }

  for (uint32_t i = 0; i < fenceCount; i++) {
    new_Fence(&(*ppFences)[i], device);
  }
  return (ERR_OK);
}

void delete_Fences(VkFence **ppFences, const uint32_t fenceCount,
                   const VkDevice device) {
  for (uint32_t i = 0; i < fenceCount; i++) {
    delete_Fence(&(*ppFences)[i], device);
  }
  free(*ppFences);
  *ppFences = NULL;
}

/* Draws a frame to the surface provided, and sets things up for the next frame
 */
uint32_t drawFrame(uint32_t *pCurrentFrame, const uint32_t maxFramesInFlight,
                   const VkDevice device, const VkSwapchainKHR swapChain,
                   const VkCommandBuffer *pCommandBuffers,
                   const VkFence *pInFlightFences,
                   const VkSemaphore *pImageAvailableSemaphores,
                   const VkSemaphore *pRenderFinishedSemaphores,
                   const VkQueue graphicsQueue, const VkQueue presentQueue) {
  /* Waits for the the current frame to finish processing */
  VkResult fenceWait = vkWaitForFences(
      device, 1, &pInFlightFences[*pCurrentFrame], VK_TRUE, UINT64_MAX);
  if (fenceWait != VK_SUCCESS) {
    errLog(FATAL, "failed to wait for fence while drawing frame: %s",
           vkstrerror(fenceWait));
    panic();
  }
  VkResult fenceResetResult =
      vkResetFences(device, 1, &pInFlightFences[*pCurrentFrame]);
  if (fenceResetResult != VK_SUCCESS) {
    errLog(FATAL, "failed to reset fence while drawing frame: %s",
           vkstrerror(fenceResetResult));
  }
  /* Gets the next image from the swap Chain */
  uint32_t imageIndex;
  VkResult nextImageResult = vkAcquireNextImageKHR(
      device, swapChain, UINT64_MAX, pImageAvailableSemaphores[*pCurrentFrame],
      VK_NULL_HANDLE, &imageIndex);
  /* If the window has been resized, the result will be an out of date error,
   * meaning that the swap chain must be resized */
  if (nextImageResult == VK_ERROR_OUT_OF_DATE_KHR) {
    return (ERR_OUTOFDATE);
  } else if (nextImageResult != VK_SUCCESS) {
    errLog(FATAL, "failed to get next frame: %s", vkstrerror(nextImageResult));
    panic();
  }

  VkSubmitInfo submitInfo = {0};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  /* Sets up for next frame */
  VkSemaphore waitSemaphores[] = {pImageAvailableSemaphores[*pCurrentFrame]};
  VkPipelineStageFlags waitStages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;

  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &pCommandBuffers[imageIndex];

  VkSemaphore signalSemaphores[] = {pRenderFinishedSemaphores[*pCurrentFrame]};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;

  VkResult queueSubmitResult = vkQueueSubmit(graphicsQueue, 1, &submitInfo,
                                             pInFlightFences[*pCurrentFrame]);
  if (queueSubmitResult != VK_SUCCESS) {
    errLog(FATAL, "failed to submit queue: %s", vkstrerror(queueSubmitResult));
    panic();
  }

  /* Present frame to screen */
  VkPresentInfoKHR presentInfo = {0};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;

  VkSwapchainKHR swapChains[] = {swapChain};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapChains;

  presentInfo.pImageIndices = &imageIndex;

  vkQueuePresentKHR(presentQueue, &presentInfo);
  /* Increments the current frame by one */ /* TODO come up with more graceful
                                               solution */
  *pCurrentFrame = (*pCurrentFrame + 1) % maxFramesInFlight;
  return (ERR_OK);
}

/* Deletes a VkSurfaceKHR */
void delete_Surface(VkSurfaceKHR *pSurface, const VkInstance instance) {
  vkDestroySurfaceKHR(instance, *pSurface, NULL);
  *pSurface = VK_NULL_HANDLE;
}

/* Gets the extent of the given GLFW window */
ErrVal getWindowExtent(VkExtent2D *pExtent, GLFWwindow *pWindow) {
  int width;
  int height;
  glfwGetFramebufferSize(pWindow, &width, &height);
  pExtent->width = (uint32_t)width;
  pExtent->height = (uint32_t)height;
  return (ERR_OK);
}

/* Creates a new window using the GLFW library. */
ErrVal new_GLFWwindow(GLFWwindow **ppGLFWwindow) {
  /* Not resizable */
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  *ppGLFWwindow =
      glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, APPNAME, NULL, NULL);
  if (*ppGLFWwindow == NULL) {
    errLog(ERROR, "failed to create GLFW window");
    return (ERR_UNKNOWN);
  }
  return (ERR_OK);
}

/* Creates a new window surface using the glfw libraries. This must be deleted
 * with the delete_Surface function*/
ErrVal new_SurfaceFromGLFW(VkSurfaceKHR *pSurface, GLFWwindow *pWindow,
                           const VkInstance instance) {
  VkResult res = glfwCreateWindowSurface(instance, pWindow, NULL, pSurface);
  if (res != VK_SUCCESS) {
    errLog(FATAL, "failed to create surface, quitting");
    panic();
  }
  return (ERR_OK);
}

/* returns any errors encountered. Finds the index of the correct type of memory
 * to allocate for a given physical device using the bits and flags that are
 * requested. */
ErrVal getMemoryTypeIndex(uint32_t *memoryTypeIndex,
                          const uint32_t memoryTypeBits,
                          const VkMemoryPropertyFlags memoryPropertyFlags,
                          const VkPhysicalDevice physicalDevice) {
  /* Retrieve memory properties */
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
  /* Check each memory type to see if it conforms to our requirements */
  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if ((memoryTypeBits &
         (1 << i)) && /* TODO figure out what's going on over here */
        (memProperties.memoryTypes[i].propertyFlags & memoryPropertyFlags) ==
            memoryPropertyFlags) {
      *memoryTypeIndex = i;
      return (ERR_OK);
    }
  }
  errLog(ERROR, "failed to find suitable memory type");
  return (ERR_MEMORY);
}

ErrVal new_VertexBuffer(VkBuffer *pBuffer, VkDeviceMemory *pBufferMemory,
                        const struct Vertex *pVertices,
                        const uint32_t vertexCount, const VkDevice device,
                        const VkPhysicalDevice physicalDevice,
                        const VkCommandPool commandPool, const VkQueue queue) {
  /* Construct staging buffers */
  VkDeviceSize bufferSize = sizeof(struct Vertex) * vertexCount;
  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  VkResult stagingBufferCreateResult = new_Buffer_DeviceMemory(
      &stagingBuffer, &stagingBufferMemory, bufferSize, physicalDevice, device,
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  if (stagingBufferCreateResult != VK_SUCCESS) {
    errLog(ERROR,
           "failed to create vertex buffer: failed to create staging buffer");
    return (stagingBufferCreateResult);
  }

  /*TODO copy data to memory */
  /* Map memory and copy the vertices to the staging buffer */
  void *data;
  VkResult mapMemoryResult =
      vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);

  /* On failure */
  if (mapMemoryResult != VK_SUCCESS) {
    /* Delete the temporary staging buffers */
    errLog(ERROR, "failed to create vertex buffer, could not map memory: %s",
           vkstrerror(mapMemoryResult));
    delete_Buffer(&stagingBuffer, device);
    delete_DeviceMemory(&stagingBufferMemory, device);
    return (ERR_MEMORY);
  }

  /* If it was successful, go on and actually copy it, making sure to unmap once
   * done */
  memcpy(data, pVertices, (size_t)bufferSize);
  vkUnmapMemory(device, stagingBufferMemory);

  /* Create vertex buffer and allocate memory for it */
  VkResult vertexBufferCreateResult = new_Buffer_DeviceMemory(
      pBuffer, pBufferMemory, bufferSize, physicalDevice, device,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  /* Handle errors */
  if (vertexBufferCreateResult != VK_SUCCESS) {
    /* Delete the temporary staging buffers */
    errLog(ERROR, "failed to create vertex buffer");
    delete_Buffer(&stagingBuffer, device);
    delete_DeviceMemory(&stagingBufferMemory, device);
    return (vertexBufferCreateResult);
  }

  /* Copy the data over from the staging buffer to the vertex buffer */
  copyBuffer(*pBuffer, stagingBuffer, bufferSize, commandPool, queue, device);

  /* Delete the temporary staging buffers */
  delete_Buffer(&stagingBuffer, device);
  delete_DeviceMemory(&stagingBufferMemory, device);

  return (ERR_OK);
}

ErrVal new_Buffer_DeviceMemory(VkBuffer *pBuffer, VkDeviceMemory *pBufferMemory,
                               const VkDeviceSize size,
                               const VkPhysicalDevice physicalDevice,
                               const VkDevice device,
                               const VkBufferUsageFlags usage,
                               const VkMemoryPropertyFlags properties) {
  VkBufferCreateInfo bufferInfo = {0};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  /* Create buffer */
  VkResult bufferCreateResult =
      vkCreateBuffer(device, &bufferInfo, NULL, pBuffer);
  if (bufferCreateResult != VK_SUCCESS) {
    errLog(ERROR, "failed to create buffer: %s",
           vkstrerror(bufferCreateResult));
    return (ERR_UNKNOWN);
  }
  /* Allocate memory for buffer */
  VkMemoryRequirements memoryRequirements;
  vkGetBufferMemoryRequirements(device, *pBuffer, &memoryRequirements);

  VkMemoryAllocateInfo allocateInfo = {0};
  allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocateInfo.allocationSize = memoryRequirements.size;
  /* Get the type of memory required, handle errors */
  ErrVal getMemoryTypeRetVal = getMemoryTypeIndex(
      &allocateInfo.memoryTypeIndex, memoryRequirements.memoryTypeBits,
      properties, physicalDevice);
  if (getMemoryTypeRetVal != ERR_OK) {
    errLog(ERROR, "failed to get type of memory to allocate");
    return (ERR_MEMORY);
  }

  /* Actually allocate memory */
  VkResult memoryAllocateResult =
      vkAllocateMemory(device, &allocateInfo, NULL, pBufferMemory);
  if (memoryAllocateResult != VK_SUCCESS) {
    errLog(ERROR, "failed to allocate memory for buffer: %s",
           vkstrerror(memoryAllocateResult));
    return (ERR_ALLOCFAIL);
  }
  vkBindBufferMemory(device, *pBuffer, *pBufferMemory, 0);
  return (ERR_OK);
}

ErrVal copyBuffer(VkBuffer destinationBuffer, const VkBuffer sourceBuffer,
                  const VkDeviceSize size, const VkCommandPool commandPool,
                  const VkQueue queue, const VkDevice device) {

  VkCommandBuffer copyCommandBuffer;
  ErrVal beginResult = new_begin_OneTimeSubmitCommandBuffer(
      &copyCommandBuffer, device, commandPool);

  if (beginResult != VK_SUCCESS) {
    errLog(ERROR, "failed to begin command buffer");
    return (beginResult);
  }

  VkBufferCopy copyRegion = {0};
  copyRegion.size = size;
  copyRegion.srcOffset = 0;
  copyRegion.dstOffset = 0;
  vkCmdCopyBuffer(copyCommandBuffer, sourceBuffer, destinationBuffer, 1,
                  &copyRegion);

  ErrVal endResult = delete_end_OneTimeSubmitCommandBuffer(
      &copyCommandBuffer, device, queue, commandPool);

  if (endResult != VK_SUCCESS) {
    errLog(ERROR, "failed to end command buffer");
    return (endResult);
  }

  return (ERR_OK);
}

void delete_Buffer(VkBuffer *pBuffer, const VkDevice device) {
  vkDestroyBuffer(device, *pBuffer, NULL);
  *pBuffer = VK_NULL_HANDLE;
}

void delete_DeviceMemory(VkDeviceMemory *pDeviceMemory, const VkDevice device) {
  vkFreeMemory(device, *pDeviceMemory, NULL);
  *pDeviceMemory = VK_NULL_HANDLE;
}

/*
 * Allocates, creates and begins one command buffer to be used.
 * Must be ended with delete_end_OneTimeSubmitCommandBuffer
 */
ErrVal new_begin_OneTimeSubmitCommandBuffer(VkCommandBuffer *pCommandBuffer,
                                            const VkDevice device,
                                            const VkCommandPool commandPool) {
  VkCommandBufferAllocateInfo allocateInfo = {0};
  allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocateInfo.commandPool = commandPool;
  allocateInfo.commandBufferCount = 1;

  VkResult allocateResult =
      vkAllocateCommandBuffers(device, &allocateInfo, pCommandBuffer);
  if (allocateResult != VK_SUCCESS) {
    errLog(ERROR, "failed to allocate command buffers: %s",
           vkstrerror(allocateResult));
    return (ERR_MEMORY);
  }

  VkCommandBufferBeginInfo beginInfo = {0};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  VkResult res = vkBeginCommandBuffer(*pCommandBuffer, &beginInfo);
  if (res != VK_SUCCESS) {
    errLog(ERROR, "failed to create command buffer: %s", vkstrerror(res));
    return (ERR_UNKNOWN);
  }

  return (ERR_OK);
}

/*
 * Ends, submits, and deletes one command buffer that was previously created in
 * new_begin_OneTimeSubmitCommandBuffer
 */
ErrVal delete_end_OneTimeSubmitCommandBuffer(VkCommandBuffer *pCommandBuffer,
                                             const VkDevice device,
                                             const VkQueue queue,
                                             const VkCommandPool commandPool) {
  ErrVal retVal = ERR_OK;
  VkResult bufferEndResult = vkEndCommandBuffer(*pCommandBuffer);
  if (bufferEndResult != VK_SUCCESS) {
    /* Clean up resources */
    errLog(ERROR, "failed to end one time submit command buffer: %s",
           vkstrerror(bufferEndResult));
    retVal = ERR_UNKNOWN;
    goto FREEALL;
  }

  VkSubmitInfo submitInfo = {0};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = pCommandBuffer;

  VkResult queueSubmitResult =
      vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
  if (queueSubmitResult != VK_SUCCESS) {
    errLog(ERROR,
           "failed to submit one time submit command buffer to queue: %s",
           vkstrerror(queueSubmitResult));
    retVal = ERR_UNKNOWN;
    goto FREEALL;
  }
/* Deallocate the buffer */
FREEALL:
  vkQueueWaitIdle(queue);
  vkFreeCommandBuffers(device, commandPool, 1, pCommandBuffer);
  *pCommandBuffer = VK_NULL_HANDLE;
  return (retVal);
}

ErrVal copyToDeviceMemory(VkDeviceMemory *pDeviceMemory,
                          const VkDeviceSize deviceSize, const void *source,
                          const VkDevice device) {
  void *data;
  VkResult mapMemoryResult =
      vkMapMemory(device, *pDeviceMemory, 0, deviceSize, 0, &data);

  /* On failure */
  if (mapMemoryResult != VK_SUCCESS) {
    errLog(ERROR, "failed to copy to device memory: failed to map memory: %s",
           vkstrerror(mapMemoryResult));
    return (ERR_MEMORY);
  }

  /* If it was successful, go on and actually copy it, making sure to unmap once
   * done */
  memcpy(data, source, (size_t)deviceSize);
  vkUnmapMemory(device, *pDeviceMemory);
  return (ERR_OK);
}
