#include "engine.hh"
#include "src/core/vertex.hh"
#include <glm/ext/matrix_float4x4.hpp>
#include <type_traits>

void Engine::init()
{
  m_window.     init();
  m_context.    init(m_window, enable_vsync);
  m_descriptor. init(m_context, frames_in_flight);
  m_pipeline.   init(m_context, m_descriptor);
  m_command.    init(m_context, frames_in_flight);
  m_resourceManager.init(registery, m_context, m_descriptor, m_command);
  m_render.     init(m_context, m_command);

  input.init(m_window);
}

bool Engine::running()
{
  return !m_window.shouldClose();
}

void Engine::close() 
{
  m_window.setShouldClose(true);
}

void Engine::render()
{
  m_window.pollEvents();
  input.update();
    m_render.draw(m_context, m_window, &m_command, m_pipeline, m_descriptor, m_resourceManager);
    deltaTime = m_render.deltaTime;
}

void Engine::destroy()
{
  m_context.deviceWait();

  m_resourceManager.destroy();
  m_render.destroy();
  m_command.destroy();
  m_pipeline.destroy();
  m_descriptor.destroy();
  m_context.destroy();
  m_window.destroy();
}
