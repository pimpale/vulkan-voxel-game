#include "camera.h"
#include "math.h"

// world up is just the unit y vector
const static vec3 worldup = {0.0f, 1.0f, 0.0f};

static CameraBasis new_CameraBasis(const float pitch, const float yaw) {
  CameraBasis cb;

  // calculate front vector from yaw and pitch
  // note that front actually points in the opposite direction as the camera
  // view
  cb.front[0] = cosf(yaw) * cosf(pitch);
  cb.front[1] = sinf(pitch);
  cb.front[2] = sinf(yaw) * cosf(pitch);
  vec3_norm(cb.front, cb.front);

  // calculate others from via gram schmidt process
  vec3_mul_cross(cb.right, cb.front, worldup);
  vec3_norm(cb.right, cb.right);
  vec3_mul_cross(cb.up, cb.right, cb.front);
  vec3_norm(cb.up, cb.up);

  return cb;
}

static void calculate_projection_matrix(mat4x4 projection_matrix,
                                        const VkExtent2D dimensions) {
  float fov = RADIANS(90.0f);
  float aspect_ratio = (float)dimensions.width / (float)dimensions.height;

  // set near and far to 0.01 and 100.0 respectively
  mat4x4_perspective(projection_matrix, fov, aspect_ratio, 0.01f, 1000.0f);
}
Camera new_Camera(const vec3 loc, const VkExtent2D dimensions) {
  Camera cam;
  vec3_dup(cam.pos, loc);

  cam.pitch = 0.0f;
  cam.yaw = RADIANS(-90.0f);

  cam.basis = new_CameraBasis(cam.pitch, cam.yaw);

  // set near and far to 0.01 and 100.0 respectively
  calculate_projection_matrix(cam.projection, dimensions);

  cam.fast = false;

  return cam;
}

void resizeCamera(Camera *camera, const VkExtent2D dimensions) {
  calculate_projection_matrix(camera->projection, dimensions);
}

void updateCamera(Camera *camera, GLFWwindow *pWindow) {

  if (glfwGetKey(pWindow, GLFW_KEY_TAB) == GLFW_PRESS) {
    camera->fast = !camera->fast;
  }

  float movscale = 0.02f;
  if (camera->fast) {
    movscale *= 10;
  }

  if (glfwGetKey(pWindow, GLFW_KEY_W) == GLFW_PRESS) {
    vec3 delta_pos;
    vec3_mul_cross(delta_pos, camera->basis.right, worldup);
    vec3_norm(delta_pos, delta_pos);
    vec3_scale(delta_pos, delta_pos, movscale);
    vec3_add(camera->pos, camera->pos, delta_pos);
  }
  if (glfwGetKey(pWindow, GLFW_KEY_S) == GLFW_PRESS) {
    vec3 delta_pos;
    vec3_mul_cross(delta_pos, camera->basis.right, worldup);
    vec3_norm(delta_pos, delta_pos);
    vec3_scale(delta_pos, delta_pos, -movscale);
    vec3_add(camera->pos, camera->pos, delta_pos);
  }
  if (glfwGetKey(pWindow, GLFW_KEY_A) == GLFW_PRESS) {
    vec3 delta_pos;
    vec3_scale(delta_pos, camera->basis.right, movscale);
    vec3_add(camera->pos, camera->pos, delta_pos);
  }
  if (glfwGetKey(pWindow, GLFW_KEY_D) == GLFW_PRESS) {
    vec3 delta_pos;
    vec3_scale(delta_pos, camera->basis.right, -movscale);
    vec3_add(camera->pos, camera->pos, delta_pos);
  }
  if (glfwGetKey(pWindow, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(pWindow, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS) {
    vec3 delta_pos;
    vec3_scale(delta_pos, worldup, movscale);
    vec3_add(camera->pos, camera->pos, delta_pos);
  }
  if (glfwGetKey(pWindow, GLFW_KEY_SPACE) == GLFW_PRESS) {
    vec3 delta_pos;
    vec3_scale(delta_pos, worldup, -movscale);
    vec3_add(camera->pos, camera->pos, delta_pos);
  }

  double x;
  double y;
  glfwGetCursorPos(pWindow, &x, &y);

  double dX = x - camera->pX;
  double dY = y - camera->pY;

  camera->pX = x;
  camera->pY = y;

  float rotscale = 0.01f;

  camera->yaw += (float)dX * rotscale;
  camera->pitch -= (float)dY * rotscale;

  // clamp camera->pitch between 89 degrees
  camera->pitch = fminf(camera->pitch, RADIANS(89.9f));
  camera->pitch = fmaxf(camera->pitch, RADIANS(-89.9f));

  // rebuild basis vectors
  camera->basis = new_CameraBasis(camera->pitch, camera->yaw);
}

void getMvpCamera(mat4x4 mvp, const Camera *camera) {
  // the place we're looking at is in the opposite direction as front
  vec3 look_pos;
  vec3_sub(look_pos, camera->pos, camera->basis.front);

  // calculate the view matrix by looking from our eye to center
  mat4x4 view;
  mat4x4_look_at(view, camera->pos, look_pos, worldup);

  // now set mvp to proj * view
  mat4x4_mul(mvp, camera->projection, view);
}
