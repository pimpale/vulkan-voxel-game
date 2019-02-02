/* Copyright 2019 Govind Pimpale
 * util_methods.h
 *
 *  Created on: Sep 2, 2018
 *      Author: gpi
 */

#ifndef SRC_UTILS_H_
#define SRC_UTILS_H_

#include <stdint.h>
#include <stdio.h>

#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <linmath.h>

typedef struct {
  vec3 position;
  vec3 color;
} Vertex;

typedef struct {
  vec3 translation;
  mat4x4 rotation;
} Transformation;

uint64_t getLength(FILE *f);

void initTransformation(Transformation *pTransformation);
void scaleTransformation(Transformation *pTransformation, const float scalar);
void translateTransformation(Transformation *pTransformation, const float dx,
                             const float dy, const float dz);
void rotateTransformation(Transformation *pTransformation, const float rx,
                          const float ry, const float rz);

void matFromTransformation(mat4x4 *pMatrix, Transformation transformation,
                           uint32_t height, uint32_t width);

void updateTransformation(Transformation *pTransformation, GLFWwindow *pWindow);

void findMatchingStrings(const char *const *ppData, uint32_t dataLen,
                         const char *const *ppQuery, uint32_t queryLen,
                         char **ppResult, uint32_t resultLen,
                         uint32_t *pMatches);

void readShaderFile(const char *filename, uint32_t *length, uint32_t **code);

#endif /* SRC_UTILS_H_ */
