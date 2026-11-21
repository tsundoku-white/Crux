#pragma once

#include "src/core/vertex.hh"
#include "src/vulkan/buffer.hh"
#include "src/core/pch.hh"

struct Mesh
{
  std::string           modelPath  = MODEL_DIR "cube.glb";
  std::vector<Vertex>   vertexData = {};
  std::vector<uint32_t> indexData  = {};
  uint32_t              indexCount = 0;

  Buffer vertexBuffer = {};
  Buffer indexBuffer  = {};
};
