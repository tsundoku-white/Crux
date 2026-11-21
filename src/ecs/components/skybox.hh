#pragma once

#include "src/ecs/components/mesh.hh"

struct Skybox
{
  std::string texturePath  = TEXTURE_DIR "sky.hdr";
  uint32_t    textureIndex = 0;
  Mesh        mesh {};
};
