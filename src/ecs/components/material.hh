#pragma once

#include <vector>
struct Material
{
  std::string base      = TEXTURE_DIR "proto_blue.png";
  glm::vec3   baseColor = {255, 255, 255};
  std::string normal    = TEXTURE_DIR "proto_blue.png";
  std::string roughness = TEXTURE_DIR "proto_blue.png";

  uint32_t baseIndex      = 0;
  uint32_t normalIndex    = 0;
  uint32_t roughnessIndex = 0;
};
