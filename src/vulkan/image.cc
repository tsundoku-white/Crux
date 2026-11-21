#include "image.hh"
#include "src/core/pch.hh"
#include "src/vulkan/command.hh"
#include <bit>
#include <vulkan/vulkan_core.h>

namespace
{
uint32_t bytesPerPixel(VkFormat f)
{
  switch (f)
  {
    case VK_FORMAT_R32G32B32A32_SFLOAT:
      return 16;
    case VK_FORMAT_R16G16B16A16_SFLOAT:
      return 8;
    case VK_FORMAT_R8G8B8A8_UNORM:
    case VK_FORMAT_R8G8B8A8_SRGB:
    case VK_FORMAT_B8G8R8A8_UNORM:
    case VK_FORMAT_B8G8R8A8_SRGB:
      return 4;
    default:
      throw std::runtime_error("bytesPerPixel: unsupported format!");
  }
}

// can this format be blitted with linear filtering (needed for mip generation)?
bool supportsLinearBlit(VkPhysicalDevice gpu, VkFormat fmt)
{
  constexpr VkFormatFeatureFlags need = VK_FORMAT_FEATURE_BLIT_SRC_BIT
                                        | VK_FORMAT_FEATURE_BLIT_DST_BIT
                                        | VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;
  VkFormatProperties             props;
  vkGetPhysicalDeviceFormatProperties(gpu, fmt, &props);
  return (props.optimalTilingFeatures & need) == need;
}
} // namespace

void Image::allocate(Context                 &context,
                     uint32_t                 w,
                     uint32_t                 h,
                     uint32_t                 mips,
                     VkFormat                 fmt,
                     VkImageUsageFlags        usage,
                     VmaAllocationCreateFlags flags,
                     VkSampleCountFlagBits    samples)
{
  // make sure old resources are gone.
  destroy();

  m_device         = context.getDevice();
  m_allocator      = context.getAllocator();
  m_physicalDevice = context.getPhysicalDevice();
  m_format         = fmt;
  m_mipLevels      = mips;
  m_samples        = samples;

  VkImageCreateInfo imageInfo {};
  imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType     = VK_IMAGE_TYPE_2D;
  imageInfo.extent        = {w, h, 1};
  imageInfo.mipLevels     = m_mipLevels;
  imageInfo.arrayLayers   = 1;
  imageInfo.format        = m_format;
  imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage         = usage;
  imageInfo.samples       = m_samples;
  imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

  VmaAllocationCreateInfo allocInfo {};
  allocInfo.usage         = VMA_MEMORY_USAGE_AUTO;
  allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
  allocInfo.flags         = flags;

  if (vmaCreateImage(m_allocator, &imageInfo, &allocInfo, &m_image, &m_alloc, nullptr)
      != VK_SUCCESS)
    throw std::runtime_error("failed to create image!");
}

void Image::createView(VkImageAspectFlags aspect)
{
  destroyView();

  VkImageViewCreateInfo viewInfo {};
  viewInfo.sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image            = m_image;
  viewInfo.viewType         = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format           = m_format;
  viewInfo.subresourceRange = {aspect, 0, m_mipLevels, 0, 1};
  if (vkCreateImageView(m_device, &viewInfo, nullptr, &m_view) != VK_SUCCESS)
    throw std::runtime_error("failed to create image view!");
}

void Image::createSampler()
{
  destroySampler();

  VkPhysicalDeviceFeatures supportedFeatures;
  vkGetPhysicalDeviceFeatures(m_physicalDevice, &supportedFeatures);

  VkPhysicalDeviceProperties properties {};
  vkGetPhysicalDeviceProperties(m_physicalDevice, &properties);

  float          maxAniso = properties.limits.maxSamplerAnisotropy;
  const VkFilter filter   = VK_FILTER_LINEAR;

  VkSamplerCreateInfo s {};
  s.sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  s.magFilter    = filter;
  s.minFilter    = filter;
  s.mipmapMode   = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  s.addressModeU = s.addressModeV = s.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  s.borderColor                                    = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  s.mipLodBias                                     = 0.0f;
  s.minLod                                         = 0.0f;
  s.maxLod                                         = VK_LOD_CLAMP_NONE;
  s.anisotropyEnable                               = VK_TRUE;
  s.maxAnisotropy                                  = maxAniso;
  if (vkCreateSampler(m_device, &s, nullptr, &m_sampler) != VK_SUCCESS)
    throw std::runtime_error("failed to create sampler!");
}

void Image::transition(VkCommandBuffer       cmd,
                       VkImageLayout         from,
                       VkImageLayout         to,
                       VkPipelineStageFlags2 srcStage,
                       VkAccessFlags2        srcAccess,
                       VkPipelineStageFlags2 dstStage,
                       VkAccessFlags2        dstAccess,
                       uint32_t              baseMip,
                       uint32_t              levelCount,
                       VkImageAspectFlags    aspect)
{
  VkImageMemoryBarrier2 b {};
  b.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
  b.srcStageMask        = srcStage;
  b.srcAccessMask       = srcAccess;
  b.dstStageMask        = dstStage;
  b.dstAccessMask       = dstAccess;
  b.oldLayout           = from;
  b.newLayout           = to;
  b.srcQueueFamilyIndex = b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  b.image                                       = m_image;
  b.subresourceRange                            = {aspect, baseMip, levelCount, 0, 1};

  VkDependencyInfo dep {};
  dep.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
  dep.imageMemoryBarrierCount = 1;
  dep.pImageMemoryBarriers    = &b;
  vkCmdPipelineBarrier2(cmd, &dep);
}

void Image::recordMipChain(VkCommandBuffer cmd, uint32_t width, uint32_t height, uint32_t mips)
{
  int32_t mipW = static_cast<int32_t>(width);
  int32_t mipH = static_cast<int32_t>(height);

  for (uint32_t i = 1; i < mips; ++i)
  {
    transition(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
               VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
               VK_PIPELINE_STAGE_2_BLIT_BIT, VK_ACCESS_2_TRANSFER_READ_BIT, i - 1, 1);

    const int32_t nextW = mipW > 1 ? mipW / 2 : 1;
    const int32_t nextH = mipH > 1 ? mipH / 2 : 1;

    VkImageBlit blit {};
    blit.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, i - 1, 0, 1};
    blit.srcOffsets[1]  = {mipW, mipH, 1};
    blit.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, i, 0, 1};
    blit.dstOffsets[1]  = {nextW, nextH, 1};
    vkCmdBlitImage(cmd, m_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_image,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

    transition(cmd, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
               VK_PIPELINE_STAGE_2_BLIT_BIT, VK_ACCESS_2_NONE,
               VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, i - 1,
               1);

    mipW = nextW;
    mipH = nextH;
  }

  transition(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
             VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
             VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, mips - 1,
             1);
}

void Image::createFromPixels(Context    &context,
                             Command    &command,
                             const void *rgba,
                             uint32_t    width,
                             uint32_t    height,
                             VkFormat    format)
{
  const VkDeviceSize imageSize = static_cast<VkDeviceSize>(width) * height * bytesPerPixel(format);

  const bool     canMip = supportsLinearBlit(context.getPhysicalDevice(), format);
  const uint32_t mips = canMip ? static_cast<uint32_t>(std::bit_width(std::max(width, height))) : 1;

  allocate(context, width, height, mips, format,
           VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
             | VK_IMAGE_USAGE_SAMPLED_BIT);

  Buffer staging;
  staging.createDynamic(context, BufferKind::Texture, imageSize);
  staging.update(rgba, imageSize);

  VkCommandBuffer cmd = command.onceStart();

  transition(cmd, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
             VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE, VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT,
             VK_ACCESS_2_TRANSFER_WRITE_BIT, 0, mips);

  VkBufferImageCopy region {};
  region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
  region.imageExtent      = {width, height, 1};
  vkCmdCopyBufferToImage(cmd, staging.handle(), m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                         &region);

  recordMipChain(cmd, width, height, mips);

  command.onceEnd(cmd);

  createView(VK_IMAGE_ASPECT_COLOR_BIT);
  createSampler();
}

void Image::destroyView()
{
  if (m_view != VK_NULL_HANDLE)
    vkDestroyImageView(m_device, m_view, nullptr);
  m_view = VK_NULL_HANDLE;
}

void Image::destroySampler()
{
  if (m_sampler != VK_NULL_HANDLE)
    vkDestroySampler(m_device, m_sampler, nullptr);
  m_sampler = VK_NULL_HANDLE;
}

// free resources.
void Image::destroy()
{
  destroySampler();
  destroyView();
  if (m_image != VK_NULL_HANDLE)
    vmaDestroyImage(m_allocator, m_image, m_alloc);

  m_image     = VK_NULL_HANDLE;
  m_alloc     = VK_NULL_HANDLE;
  m_format    = VK_FORMAT_UNDEFINED;
  m_mipLevels = 0;
  m_samples   = VK_SAMPLE_COUNT_1_BIT;
}
