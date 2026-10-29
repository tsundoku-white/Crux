#pragma once

#include "src/core/pch.hh"
#include "src/vulkan/command.hh"
#include "src/vulkan/context.hh"
#include <vulkan/vulkan_core.h>

enum class BufferKind : VkBufferUsageFlags {
    Vertex  = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    Index   = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
    Uniform = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
    Storage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
};

class Buffer
{
private:
  VmaAllocator    m_allocator = VK_NULL_HANDLE;
  VmaAllocation   m_alloc     = VK_NULL_HANDLE;
  VkBuffer        m_buffer    = VK_NULL_HANDLE;
  VkDeviceSize    m_size      = 0;
  void           *m_mapped    = nullptr;
  VkDeviceAddress m_address   = 0;

void create_buffer(Context &context, VkDeviceSize size,
                   VkBufferUsageFlags usage, VmaMemoryUsage memory_usage,
                   VmaAllocationCreateFlags alloc_flags);

public:
  // GPU-only buffer, filled once through a staging copy.
  void createStatic(Context &context, Command &command, BufferKind kind,
                    const void *data, VkDeviceSize size, bool device_address = false);

  // CPU-visible buffer you can update every frame.
  void createDynamic(Context &context, BufferKind kind, VkDeviceSize size,
                     bool device_address = false);

  // memcpy into the mapped memory (dynamic buffers only).
  void update(const void *data, VkDeviceSize size, VkDeviceSize offset = 0);

  // safe to call twice.
  void destroy();

  void copy(Context &context, Command &command, VkBuffer src, VkBuffer dst, VkDeviceSize size);

  VkBuffer        handle() const { return m_buffer; }
  VkDeviceSize    size() const { return m_size; }
  VkDeviceAddress address() const { return m_address; }
  bool            valid() const { return m_buffer != VK_NULL_HANDLE; }
};
