#pragma once

#include "src/core/pch.hh"
#include "src/vulkan/UBO.hh"
#include <cstdint>

class Context;

class Descriptor
{
private:
  VkDevice     m_device         = VK_NULL_HANDLE;
  VmaAllocator m_allocator      = nullptr;
  uint32_t     m_framesInFlight = 3;

  VkDescriptorPool             m_pool    = VK_NULL_HANDLE;
  VkDescriptorSetLayout        m_layout  = VK_NULL_HANDLE;
  std::vector<VkDescriptorSet> m_sets    = {};
  std::vector<VkBuffer>        m_buffers = {};
  std::vector<VmaAllocation>   m_allocs  = {};
  std::vector<void *>          m_mapped  = {};

public:
  void init(Context &context, uint32_t framesInFlight);
  void destroy();

  template <typename T>
  void uploadToGPU(uint32_t currentFrame, const T *data, uint32_t size, uint32_t offset)
  {
    memcpy(static_cast<char *>(m_mapped[currentFrame]) + offset, data, size);
    vmaFlushAllocation(m_allocator, m_allocs[currentFrame], offset, size);
  }

  VkDescriptorSetLayout               getLayout() const { return m_layout; }
  const std::vector<VkDescriptorSet> &getSets() const { return m_sets; }
};
