#pragma once
#include "src/core/pch.hh"

struct DrawCall
{
  VkBuffer  vertexBuffer = VK_NULL_HANDLE;
  VkBuffer  indexBuffer  = VK_NULL_HANDLE;
  glm::mat4 model {1.f};

  uint32_t indexCount    = 0;
  uint32_t instanceCount = 1;
  uint32_t firstIndex    = 0;
  int32_t  vertexOffset  = 0;
  uint32_t firstInstance = 0;
};
