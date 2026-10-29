#pragma once

#include "src/core/pch.hh"
#include "src/vulkan/descriptor.hh"

class Context;
class Pipeline;
class Buffer;

class Command
{
private:
  VkDevice      m_device = VK_NULL_HANDLE;
  VkCommandPool m_pool   = VK_NULL_HANDLE;

  std::vector<VkCommandBuffer> m_buffers = {};

public:
  // -- functions ---------
  void init(Context &context, uint32_t framesInFlight);
  void destroy();
  void record(uint32_t currentFrame, uint32_t imageIndex,
            Context &context, Pipeline &pipeline, Descriptor &descriptor,
            const Buffer &vertex, const Buffer &index, uint32_t index_count);
  // -- gettters ---------
  std::vector<VkCommandBuffer> getBuffers() { return m_buffers; }
  VkCommandPool getPool() const { return m_pool; }
};
