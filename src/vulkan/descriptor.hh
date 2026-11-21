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

  // per-frame object SSBO (binding 1)
  std::vector<VkBuffer>      m_ssboBuffers = {};
  std::vector<VmaAllocation> m_ssboAllocs  = {};
  std::vector<void *>        m_ssboMapped  = {};

public:
  void init(Context &context, uint32_t framesInFlight);
  void destroy();

  template <typename T>
  void uploadToGPU(uint32_t currentFrame, const T *data, uint32_t size, uint32_t offset)
  {
    memcpy(static_cast<char *>(m_mapped[currentFrame]) + offset, data, size);
    vmaFlushAllocation(m_allocator, m_allocs[currentFrame], offset, size);
  }

  // Writes `size` bytes of ObjectData into this frame's SSBO (binding 1).
  void uploadObjects(uint32_t currentFrame, const void *data, uint32_t size)
  {
    memcpy(m_ssboMapped[currentFrame], data, size);
    vmaFlushAllocation(m_allocator, m_ssboAllocs[currentFrame], 0, size);
  }

  VkDescriptorSetLayout getLayout() const { return m_layout; }
  const std::vector<VkDescriptorSet> &getSets() const { return m_sets; }
};
