/*
 * error_handle.c
 *
 *  Created on: Aug 7, 2018
 *      Author: gpi
 */

#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <vulkan/vulkan.h>

#include "constants.h"

#include "errors.h"

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
  case VK_RESULT_RANGE_SIZE:
    return ("VK_RESULT_RANGE_SIZE");
  case VK_RESULT_MAX_ENUM:
    return ("VK_RESULT_MAX_ENUM");
  }
  return ("UNKNOWN_ERROR");
}

 void errLog(ErrSeverity level, const char*message, ...) {
  char *errmsg = "unknown";
  FILE *out = stderr;

  switch (level) {
  case ERR_LEVEL_DEBUG: {
    errmsg = "debug";
    out = stdout;
    break;
  }
  case ERR_LEVEL_INFO: {
    errmsg = "info";
    out = stdout;
    break;
  }
  case ERR_LEVEL_WARN: {
    errmsg = "warn";
    out = stderr;
    break;
  }
  case ERR_LEVEL_ERROR: {
    errmsg = "error";
    out = stderr;
    break;
  }
  case ERR_LEVEL_FATAL: {
    errmsg = "fatal";
    out = stderr;
    break;
  }
  case ERR_LEVEL_UNKNOWN: {
    errmsg = "unknown";
    out = stderr;
    break;
  }
  }

  char message_formatted[MAX_PRINT_LENGTH];
  va_list args;
  va_start(args, message);
  vsnprintf(message_formatted, MAX_PRINT_LENGTH, message, args);
  va_end(args);

  fprintf(out, "%s: %s: %s\n", APPNAME, errmsg, message_formatted);
}

