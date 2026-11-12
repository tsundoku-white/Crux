#pragma once

#include "src/core/pch.hh"
#include "src/vulkan/buffer.hh"
#include "src/vulkan/command.hh"
#include "src/vulkan/context.hh"

class Image
{
private:
  VkDevice      m_device    = VK_NULL_HANDLE;
  VmaAllocator  m_allocator = VK_NULL_HANDLE;
  VmaAllocation m_alloc     = VK_NULL_HANDLE;
  VkImage       m_image     = VK_NULL_HANDLE;
  VkImageView   m_view      = VK_NULL_HANDLE;
  VkSampler     m_sampler   = VK_NULL_HANDLE;

  void transition(Command &command, VkImageLayout oldLayout, VkImageLayout newLayout);
  void copyFromBuffer(Command &command, Buffer &buffer, uint32_t width, uint32_t height);

  void swap(Image &o) noexcept
  {
    std::swap(m_device, o.m_device);
    std::swap(m_allocator, o.m_allocator);
    std::swap(m_alloc, o.m_alloc);
    std::swap(m_image, o.m_image);
    std::swap(m_view, o.m_view);
    std::swap(m_sampler, o.m_sampler);
  }

public:
  Image() = default;
  ~Image() { destroy(); }

  Image(const Image &)            = delete;
  Image &operator=(const Image &) = delete;
  Image(Image &&other) noexcept { swap(other); }
  Image &operator=(Image &&other) noexcept
  {
    if (this != &other)
    {
      destroy();
      swap(other);
    }
    return *this;
  }

  void createTexture(Context &context, Command &command, const std::string &path);

  VkImageView view() const { return m_view; }
  VkSampler   sampler() const { return m_sampler; }

  // safe to call twice.
  void destroy();
};
