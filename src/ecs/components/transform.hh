#pragma once 

#include "src/core/pch.hh"

struct Transform
{
  glm::vec3 location = {0,0,0};
  glm::quat rotation = {1,0,0,0};
  glm::vec3 scale    = {1,1,1};

  glm::vec3 forward = {0,0,-1};
  glm::vec3 right = {1,0,0};
  glm::vec3 up = {0,1,0};
};
