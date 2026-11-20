#pragma once

#include "src/core/vertex.hh"
#include "src/vulkan/buffer.hh"
#include "src/core/pch.hh"

struct Mesh
{
  std::string           modelPath    = MODEL_DIR "cube.glb";
  std::string           texturePath  = TEXTURE_DIR "proto_blue.png";
  std::vector<Vertex>   vertexData   = {};
  std::vector<uint32_t> indexData    = {};
  uint32_t              indexCount   = 0;
  uint32_t              textureIndex = 0; // slot in the bindless texture array (0 = white)

  Buffer vertexBuffer = {};
  Buffer indexBuffer  = {};
};
