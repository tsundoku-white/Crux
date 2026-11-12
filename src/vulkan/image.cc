#include "image.hh"
#include "src/core/pch.hh"
#include "src/vulkan/command.hh"
#include "stb_image.h"

void Image::createTexture(Context &context, Command &command, const std::string &path)
{
  m_device    = context.getDevice();
  m_allocator = context.getAllocator();

  // 1. load pixels
  int width = 0, height = 0, channels = 0;
  std::unique_ptr<stbi_uc, decltype(&stbi_image_free)> pixels(
      stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha), &stbi_image_free);
  if (!pixels)
    throw std::runtime_error("failed to load texture image: " + path);

  VkDeviceSize imageSize = static_cast<VkDeviceSize>(width) * height * 4;

  // 2. staging buffer (mapped, TRANSFER_SRC)
  Buffer staging;
  staging.createDynamic(context, BufferKind::Texture, imageSize);
  staging.update(pixels.get(), imageSize);
  pixels.reset();

  // 3. create GPU image
  VkImageCreateInfo imageInfo {};
  imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType     = VK_IMAGE_TYPE_2D;
  imageInfo.extent        = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};
  imageInfo.mipLevels     = 1;
  imageInfo.arrayLayers   = 1;
  imageInfo.format        = VK_FORMAT_R8G8B8A8_SRGB;
  imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

  VmaAllocationCreateInfo allocInfo {};
  allocInfo.usage         = VMA_MEMORY_USAGE_AUTO;
  allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

  if (vmaCreateImage(m_allocator, &imageInfo, &allocInfo, &m_image, &m_alloc, nullptr) !=
      VK_SUCCESS)
    throw std::runtime_error("failed to create image!");

  // 4. upload
  transition(command, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  copyFromBuffer(command, staging, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
  transition(command, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
             VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  // 5. view
  VkImageViewCreateInfo viewInfo {};
  viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image                           = m_image;
  viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format                          = VK_FORMAT_R8G8B8A8_SRGB;
  viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  viewInfo.subresourceRange.baseMipLevel   = 0;
  viewInfo.subresourceRange.levelCount     = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount     = 1;

  if (vkCreateImageView(m_device, &viewInfo, nullptr, &m_view) != VK_SUCCESS)
    throw std::runtime_error("failed to create image view!");

  // 6. sampler (anisotropy off, so no physical device or device feature needed)
  VkSamplerCreateInfo samplerInfo {};
  samplerInfo.sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter    = VK_FILTER_LINEAR;
  samplerInfo.minFilter    = VK_FILTER_LINEAR;
  samplerInfo.mipmapMode   = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.borderColor  = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerInfo.maxLod       = 0.0f;

  if (vkCreateSampler(m_device, &samplerInfo, nullptr, &m_sampler) != VK_SUCCESS)
    throw std::runtime_error("failed to create sampler!");
}

void Image::transition(Command &command, VkImageLayout oldLayout, VkImageLayout newLayout)
{
  VkImageMemoryBarrier barrier {};
  barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout                       = oldLayout;
  barrier.newLayout                       = newLayout;
  barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  barrier.image                           = m_image;
  barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel   = 0;
  barrier.subresourceRange.levelCount     = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount     = 1;

  VkPipelineStageFlags srcStage, dstStage;

  if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
  {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    srcStage              = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    dstStage              = VK_PIPELINE_STAGE_TRANSFER_BIT;
  }
  else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
           newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
  {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    srcStage              = VK_PIPELINE_STAGE_TRANSFER_BIT;
    dstStage              = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  }
  else
  {
    throw std::invalid_argument("unsupported layout transition");
  }

  VkCommandBuffer cmd = command.onceStart();
  vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
  command.onceEnd(cmd);
}

void Image::copyFromBuffer(Command &command, Buffer &buffer, uint32_t width, uint32_t height)
{
  VkBufferImageCopy region {};
  region.bufferOffset                    = 0;
  region.bufferRowLength                 = 0;
  region.bufferImageHeight               = 0;
  region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel       = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount     = 1;
  region.imageOffset                     = {0, 0, 0};
  region.imageExtent                     = {width, height, 1};

  VkCommandBuffer cmd = command.onceStart();
  vkCmdCopyBufferToImage(cmd, buffer.handle(), m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                         &region);
  command.onceEnd(cmd);
}

void Image::destroy()
{
  if (m_sampler != VK_NULL_HANDLE)
    vkDestroySampler(m_device, m_sampler, nullptr);
  if (m_view != VK_NULL_HANDLE)
    vkDestroyImageView(m_device, m_view, nullptr);
  if (m_image != VK_NULL_HANDLE)
    vmaDestroyImage(m_allocator, m_image, m_alloc);

  m_sampler = VK_NULL_HANDLE;
  m_view    = VK_NULL_HANDLE;
  m_image   = VK_NULL_HANDLE;
  m_alloc   = VK_NULL_HANDLE;
}
