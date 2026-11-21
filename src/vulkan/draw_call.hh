#pragma once
#include "src/core/pch.hh"
#include <cstdint>

constexpr uint32_t MAX_OBJECTS = 1024;

struct alignas(16) ObjectData
{
  glm::mat4 model {1.f};
  glm::vec4 baseColor {1.f};
  uint32_t  textureIndex   = 0;
  uint32_t  normalIndex    = 1;
  uint32_t  roughnessIndex = 0;
  float     pad_;
};
static_assert(sizeof(ObjectData) == 96, "must match the Slang SSBO stride");

struct DrawCall
{
  VkBuffer vertexBuffer = VK_NULL_HANDLE;
  VkBuffer indexBuffer  = VK_NULL_HANDLE;

  uint32_t indexCount    = 0;
  uint32_t instanceCount = 1;
  uint32_t firstIndex    = 0;
  int32_t  vertexOffset  = 0;
  uint32_t firstInstance = 0;
};
