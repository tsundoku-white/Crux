#pragma once

#include "src/core/vertex.hh"
#include "src/vulkan/buffer.hh"
#include <cstdint>

struct Mesh
{
  std::vector<Vertex>   vertexData = {};
  std::vector<uint32_t> indexData  = {};
  uint32_t              indexCount = 0;

  Buffer vertexBuffer = {};
  Buffer indexBuffer  = {};
};
