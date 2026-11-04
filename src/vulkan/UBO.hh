#pragma once
#include "src/core/pch.hh"

struct UBO
{
  glm::mat4 projectionMatrix {1.f};
  glm::mat4 viewMatrix {1.f};
  glm::mat4 model {1.f};
};

