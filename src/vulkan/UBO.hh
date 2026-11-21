#pragma once
#include "src/core/pch.hh"

// the Uniform Buffer Object
struct UBO
{
  glm::mat4 projectionMatrix {1.f};
  glm::mat4 viewMatrix {1.f};
  glm::vec4 cameraPos;
  uint32_t  debugView = 0;
  uint32_t  _pad[3];
};
