#pragma once

#include "src/core/pch.hh"
#include "src/vulkan/buffer.hh"
#include "src/vulkan/command.hh"
#include "src/vulkan/context.hh"
#include <vulkan/vulkan_core.h>

class Image
{
private:
  VkDevice              m_device    = VK_NULL_HANDLE;
  VmaAllocator          m_allocator = VK_NULL_HANDLE;
  VmaAllocation         m_alloc     = VK_NULL_HANDLE;
  VkImage               m_image     = VK_NULL_HANDLE;
  VkImageView           m_view      = VK_NULL_HANDLE;
  VkSampler             m_sampler   = VK_NULL_HANDLE;
  VkFormat              m_format    = VK_FORMAT_UNDEFINED;
  uint32_t              m_mipLevels = 0;
  VkSampleCountFlagBits m_samples   = VK_SAMPLE_COUNT_1_BIT;

  void swap(Image &o) noexcept
  {
    std::swap(m_device, o.m_device);
    std::swap(m_allocator, o.m_allocator);
    std::swap(m_alloc, o.m_alloc);
    std::swap(m_image, o.m_image);
    std::swap(m_view, o.m_view);
    std::swap(m_sampler, o.m_sampler);
    std::swap(m_format, o.m_format);
    std::swap(m_mipLevels, o.m_mipLevels);
    std::swap(m_samples, o.m_samples);
  }

  void destroyView();
  void destroySampler();

  // records the blit chain that fills mips 1..n-1 and leaves every mip in SHADER_READ_ONLY.
  void recordMipChain(VkCommandBuffer cmd, uint32_t width, uint32_t height, uint32_t mips);

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

  // uploads width * height pixels (tightly packed, layout matches `format`), builds the mip chain
  // when the format supports it, creates view + sampler. `linear` picks LINEAR or NEAREST
  // mag/min filtering. Blocks until the upload finishes.
  void createFromPixels(Context &context, Command &command, const void *rgba, uint32_t width,
                        uint32_t height, VkFormat format);

  // (re)allocates the image. Any previous image, view and sampler are destroyed first.
  void allocate(Context &ctx, uint32_t w, uint32_t h, uint32_t mips, VkFormat fmt,
                VkImageUsageFlags usage, VmaAllocationCreateFlags flags = 0,
                VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT);

  void createView(VkImageAspectFlags aspect);
  void createSampler();
  void transition(VkCommandBuffer cmd, VkImageLayout from, VkImageLayout to,
                  VkPipelineStageFlags2 srcStage, VkAccessFlags2 srcAccess,
                  VkPipelineStageFlags2 dstStage, VkAccessFlags2 dstAccess, uint32_t baseMip,
                  uint32_t levelCount, VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT);

  // -- getters ----------
  VkImage               getImage() const { return m_image; }
  VkImageView           getView() const { return m_view; }
  VkSampler             getSampler() const { return m_sampler; }
  VkFormat              getFormat() const { return m_format; }
  uint32_t              getMipLevels() const { return m_mipLevels; }
  VkSampleCountFlagBits getSamples() const { return m_samples; }

  void destroy(); // safe to call twice
};
