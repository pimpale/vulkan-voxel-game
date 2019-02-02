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
  pTransformation->translation[2] = 0.0f;
  quat_identity(pTransformation->rotation);
}

void translateTransformation(Transformation *pTransformation, const float dx,
                             const float dy, const float dz) {
  pTransformation->translation[0] += dx;
  pTransformation->translation[1] += dy;
  pTransformation->translation[2] += dz;
}

void rotateTransformation(Transformation *pTransformation, const float rx,
                          const float ry, const float rz) {
  quat qx;
  quat qy;
  quat qz;
  quat_rotate(qx, rx, (vec3){1.0f, 0.0f, 0.0f});
  quat_rotate(qy, ry, (vec3){0.0f, 1.0f, 0.0f});
  quat_rotate(qz, rz, (vec3){0.0f, 0.0f, 1.0f});
  quat_mul(pTransformation->rotation, pTransformation->rotation, qx);
  quat_mul(pTransformation->rotation, pTransformation->rotation, qy);
  quat_mul(pTransformation->rotation, pTransformation->rotation, qz);
}

void updateTransformation(Transformation *pTransformation,
                          GLFWwindow *pWindow) {
  float dx = 0.0f;
  float dy = 0.0f;
  float dz = 0.0f;
  if (glfwGetKey(pWindow, GLFW_KEY_A) == GLFW_PRESS) {
    dx += 0.01f;
  }
  if (glfwGetKey(pWindow, GLFW_KEY_D) == GLFW_PRESS) {
    dx += -0.01f;
  }
  if (glfwGetKey(pWindow, GLFW_KEY_W) == GLFW_PRESS) {
    dz += 0.01f;
  }
  if (glfwGetKey(pWindow, GLFW_KEY_S) == GLFW_PRESS) {
    dz -= 0.01f;
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
                           int height, int width) {

  /* Begin with our mvp matrices */
  mat4x4 view;
  mat4x4 model;
  mat4x4 projection;

  mat4x4_identity(model);

  /* rotate unit vector for lookAt */
  vec3 lookAtPoint = {0.0f, 0.0f, 1.0f};
  quat_mul_vec3(lookAtPoint, transformation.rotation, lookAtPoint);

  mat4x4_look_at(
      view, transformation.translation, /*  Position of camera in world space */
      lookAtPoint,              /* Where we want the camera to look at */
      (vec3){0.0f, -1.0f, 0.0f} /* What is up for the camera */
                                /* (negative 1 because vulkan reverses it */
  );

  mat4x4_perspective(projection, RADIANS(60.0f),       /* Field of vision */
                     ((float)width) / ((float)height), /* Aspect ratio*/
                     0.1f, 100.0f);                    /* The display range */

  mat4x4_mul(*pMatrix, model, view);
  mat4x4_mul(*pMatrix, *pMatrix, projection);
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
