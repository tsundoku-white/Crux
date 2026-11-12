#pragma once

#include "src/core/pch.hh"
#include "src/vulkan/descriptor.hh"
#include "src/vulkan/draw_call.hh"
#include <span>

class Context;
class Pipeline;
class Buffer;
struct DrawCall;

class Command
{
private:
  VkDevice      m_device = VK_NULL_HANDLE;
  VkCommandPool m_pool   = VK_NULL_HANDLE;
  VkQueue       m_queue  = VK_NULL_HANDLE;

  std::vector<VkCommandBuffer> m_buffers = {};

public:
  // -- functions ---------
  void            init(Context &context, uint32_t framesInFlight);
  void            destroy();
  VkCommandBuffer onceStart();
  void            onceEnd(VkCommandBuffer commandBuffer);
  void record(uint32_t currentFrame, uint32_t imageIndex, Context &context, Pipeline &pipeline,
              Descriptor &descriptor, std::span<const DrawCall> drawCalls);
  // -- gettters ---------
  std::vector<VkCommandBuffer> getBuffers() { return m_buffers; }
  VkCommandPool                getPool() const { return m_pool; }
};
