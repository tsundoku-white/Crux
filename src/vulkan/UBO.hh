#pragma once
#include "src/core/pch.hh"

// the Uniform Buffer Object
struct UBO
{
  glm::mat4 projectionMatrix {1.f};
  glm::mat4 viewMatrix {1.f};
};
