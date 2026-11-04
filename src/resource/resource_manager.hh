#pragma once

#include "src/ecs/register.hh"
#include "src/vulkan/UBO.hh"
#include "src/vulkan/context.hh"
#include "src/vulkan/descriptor.hh"
#include "src/vulkan/draw_call.hh"
#include <cstdint>
#include <vector>

class Command;

class ResourceManager
{
private:
  Registry   *m_registry;
  Descriptor *m_descriptor;
  Context    *m_context;
  Command    *m_command;
  UBO         m_ubo;
  uint32_t    m_currentFrame = 0;

  // camera
  bool  m_prespectiveUpdate = true;
  float m_aspectRatio       = 16.f / 9.f;

  std::vector<EntityID> m_entities  = {};
  std::vector<DrawCall> m_drawCalls = {};

  // -- functions ---------
  void cachedModels();
  void createModels();
  // componets
  void updateCamera();
  void updateTransform();
  void updateModel();

  bool loadMeshData(std::vector<Vertex> &vertex, std::vector<uint32_t> &index, const std::string &path);

  void buildDrawCalls();

public:
  // -- functions ---------
  void init(Registry &registry, Context &context, Descriptor &descriptor, Command &command);
  void update(uint32_t currentFrame);
  void destroy();

  void                      setProjectionUpdate(bool value) { m_prespectiveUpdate = value; }
  std::span<const DrawCall> drawCalls() const { return m_drawCalls; }
};
