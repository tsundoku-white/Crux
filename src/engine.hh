#pragma once

#include "src/platform/window.hh"
#include "src/platform/input.hh"
#include "src/vulkan/context.hh"
#include "src/vulkan/descriptor.hh"
#include "src/vulkan/pipeline.hh"
#include "src/vulkan/command.hh"
#include "src/vulkan/buffer.hh"
#include "src/vulkan/render.hh"
#include <cstdint>

class Engine
{
private:
  // rendering
  Window     m_window;
  Context    m_context;
  Pipeline   m_pipeline;
  Command    m_command;
  Descriptor m_descriptor;
  Render     m_render;

  Buffer   m_vertex_buffer;
  Buffer   m_index_buffer;
  uint32_t m_index_count = 0;

public:
  // -- setup ----------
  void init();
  bool running();
  void destroy();
  void close();
  void render();
  // -- Input ------------
  Input input;

  // frames
  uint32_t frames_in_flight = 3;

  // msaa
  bool     enable_msaa = true;
  uint32_t sample_msaa = 8;

  // vsync
  bool enable_vsync = true;
};
