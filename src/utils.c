/*
 * util_methods.c
 *
 *  Created on: Sep 15, 2018
 *      Author: gpi
 */

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "linmath.h"

#include "constants.h"
#include "errors.h"

#include "utils.h"

void initTransformation(Transformation *pTransformation) {
  pTransformation->translation[0] = 0.0f;
  pTransformation->translation[1] = 0.0f;
  pTransformation->translation[2] = 3.0f;
  mat4x4_identity(pTransformation->rotation);
}

void translateTransformation(Transformation *pTransformation, const float dx,
                             const float dy, const float dz) {
  pTransformation->translation[0] += dx;
  pTransformation->translation[1] += dy;
  pTransformation->translation[2] += dz;
}

void rotateTransformation(Transformation *pTransformation, const float rx,
                          const float ry, const float rz) {
  mat4x4_rotate_X(pTransformation->rotation, pTransformation->rotation, rx);
  mat4x4_rotate_Y(pTransformation->rotation, pTransformation->rotation, ry);
  mat4x4_rotate_Z(pTransformation->rotation, pTransformation->rotation, rz);
}

void updateTransformation(Transformation *pTransformation,
                          GLFWwindow *pWindow) {
  float dx = 0.0f;
  float dy = 0.0f;
  float dz = 0.0f;
  if (glfwGetKey(pWindow, GLFW_KEY_A) == GLFW_PRESS) {
    dx -= 0.01f;
  }
  if (glfwGetKey(pWindow, GLFW_KEY_D) == GLFW_PRESS) {
    dx += 0.01f;
  }
  if (glfwGetKey(pWindow, GLFW_KEY_W) == GLFW_PRESS) {
    dy -= 0.01f;
  }
  if (glfwGetKey(pWindow, GLFW_KEY_S) == GLFW_PRESS) {
    dy += 0.01f;
  }
  if (glfwGetKey(pWindow, GLFW_KEY_Q) == GLFW_PRESS) {
    dz -= 0.01f;
  }
  if (glfwGetKey(pWindow, GLFW_KEY_E) == GLFW_PRESS) {
    dz += 0.01f;
  }
  translateTransformation(pTransformation, dx, dy, dz);

  float rx = 0.0f;
  float ry = 0.0f;
  float rz = 0.0f;

  if (glfwGetKey(pWindow, GLFW_KEY_UP) == GLFW_PRESS) {
    ry += 0.01f;
  }
  if (glfwGetKey(pWindow, GLFW_KEY_DOWN) == GLFW_PRESS) {
    ry += -0.01f;
  }
  if (glfwGetKey(pWindow, GLFW_KEY_LEFT) == GLFW_PRESS) {
    rx += 0.01f;
  }
  if (glfwGetKey(pWindow, GLFW_KEY_RIGHT) == GLFW_PRESS) {
    rx -= 0.01f;
  }
  rotateTransformation(pTransformation, rx, ry, rz);
}

void matFromTransformation(mat4x4 *pMatrix, Transformation transformation,
                           uint32_t width, uint32_t height) {
  mat4x4 view;
  mat4x4 projection;
  mat4x4_look_at(
      view, transformation.translation, /* Camera Location in world space */
      (vec3){0.0f, 0.0f, 0.0f},         /* Look towards origin */
      (vec3){0.0f, 0.0f, -1.0f});       /* Direction considered to be up */
  mat4x4_perspective(projection, RADIANS(90), /* Field of vision in radians */
                     ((float)width) / height, /* The aspect ratio */
                     0.1f, 1000.0f);          /* The 1 is in radians */
  mat4x4_mul(*pMatrix, projection, view);
}

uint64_t getLength(FILE *f) {
  /* TODO what if the file is modified as we read it? */
  int64_t currentpos = ftell(f);
  fseek(f, 0, SEEK_END);
  int64_t size = ftell(f);
  fseek(f, currentpos, SEEK_SET);
  if (size < 0) {
    LOG_ERROR(ERR_LEVEL_ERROR, "invalid file size");
    return (0);
  }
  return ((uint64_t)size);
}

/**
 * Mallocs
 */
void readShaderFile(const char *filename, uint32_t *length, uint32_t **code) {
  FILE *fp = fopen(filename, "rb");
  if (!fp) {
    LOG_ERROR(ERR_LEVEL_FATAL, "could not read file");
    PANIC();
  }
  /* We can coerce to a 32 bit, because no realistic files will be
   * greater than 2 GB */
  uint32_t filesize = (uint32_t)getLength(fp);
  uint32_t filesizepadded =
      (filesize % 4 == 0 ? filesize * 4 : (filesize + 1) * 4) / 4;

  char *str = malloc(filesizepadded);
  if (!str) {
    LOG_ERROR_ARGS(ERR_LEVEL_FATAL, "could not read shader file: %s",
                   strerror(errno));
    fclose(fp);
    PANIC();
  }

  fread(str, filesize, sizeof(char), fp);
  fclose(fp);

  /*pad data*/
  for (uint32_t i = filesize; i < filesizepadded; i++) {
    str[i] = 0;
  }

  /*cast data t array of uints*/
  *length = filesizepadded;
  *code = (uint32_t *)((void *)str);
  return;
}
