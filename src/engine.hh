#pragma once

#include "src/ecs/register.hh"
#include "src/platform/window.hh"
#include "src/platform/input.hh"
#include "src/vulkan/context.hh"
#include "src/vulkan/descriptor.hh"
#include "src/vulkan/msaa.hh"
#include "src/vulkan/pipeline.hh"
#include "src/vulkan/command.hh"
#include "src/vulkan/buffer.hh"
#include "src/resource/texture_manager.hh"
#include "src/vulkan/render.hh"
#include "src/vulkan/depth_buffer.hh"
#include "src/resource/resource_manager.hh"

class Engine
{
private:
  // rendering
  Window          m_window;
  Context         m_context;
  Pipeline        m_pipeline;
  Command         m_command;
  Descriptor      m_descriptor;
  TextureManager  m_textures;
  ResourceManager m_resourceManager;
  DepthBuffer     m_depthBuffer;
  Msaa            m_msaa;
  Render          m_render;

public:
  // -- setup ----------
  Engine();
  ~Engine();
  bool running();
  void close();
  void render();
  // -- models ------------
  Input    input;
  Registry registery;

  float    deltaTime        = 0;
  uint32_t frames_in_flight = 3;
  bool     enable_msaa      = true;
  uint32_t sample_msaa      = 8;
  bool     enable_vsync     = true;
};
