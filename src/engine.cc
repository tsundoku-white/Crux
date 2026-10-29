#include "engine.hh"
#include "src/core/vertex.hh"

void Engine::init()
{
  m_window.     init();
  m_context.    init(m_window, enable_vsync);
  m_descriptor. init(m_context, frames_in_flight);
  m_pipeline.   init(m_context, m_descriptor);
  m_command.    init(m_context, frames_in_flight);
  m_render.     init(m_context, m_command);

  input.init(m_window);

  // delete in future VVVVVV

  const glm::vec3 n {0.f, 0.f, -1.f};
  std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f, 0.f}, {0.f, 0.f}, {1.f, 1.f, 1.f}, n}, // top-left
    {{ 0.5f, -0.5f, 0.f}, {1.f, 0.f}, {1.f, 1.f, 1.f}, n}, // top-right
    {{ 0.5f,  0.5f, 0.f}, {1.f, 1.f}, {1.f, 1.f, 1.f}, n}, // bottom-right
    {{-0.5f,  0.5f, 0.f}, {0.f, 1.f}, {1.f, 1.f, 1.f}, n}, // bottom-left
  };
  std::vector<uint32_t> indices = {0, 1, 2, 2, 3, 0};
  m_index_count = (uint32_t)indices.size();

  m_vertex_buffer.createStatic(m_context, m_command, BufferKind::Vertex, vertices.data(), sizeof(Vertex) * vertices.size());
  m_index_buffer.createStatic(m_context, m_command, BufferKind::Index, indices.data(), sizeof(uint32_t) * indices.size());

  // nothing writes the UBO yet, so upload identity matrices for every frame
  UBO ubo {};
  for (uint32_t i = 0; i < frames_in_flight; ++i)
    m_descriptor.uploadToGPU(i, &ubo, sizeof(UBO));

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
    m_render.draw(m_context, m_window, &m_command, m_pipeline, m_descriptor,
                m_vertex_buffer, m_index_buffer, m_index_count);
}

void Engine::destroy()
{
  m_context.deviceWait();

  m_vertex_buffer.destroy();
  m_index_buffer.destroy();

  m_render.destroy();
  m_command.destroy();
  m_pipeline.destroy();
  m_descriptor.destroy();
  m_context.destroy();
  m_window.destroy();
}
