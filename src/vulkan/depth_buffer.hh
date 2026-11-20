#pragma once

#include "src/vulkan/command.hh"
#include "src/vulkan/context.hh"
#include "src/vulkan/image.hh"
#include <vulkan/vulkan_core.h>
class DepthBuffer
{
private:
  VkFormat m_format = {};
  Image    m_image;

  bool hasStencil(VkFormat format)
  {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
  }

public:
  void init(Context &context, Command &command, Msaa &msaa);
  void destroy();

  VkImageView getView() { return m_image.getView(); }
  VkImage     getImage() { return m_image.getImage(); }
};
