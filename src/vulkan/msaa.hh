#pragma once

#include "src/vulkan/command.hh"
#include "src/vulkan/context.hh"
#include "src/vulkan/image.hh"

class Msaa
{
private:
  Image                 m_image;
  VkSampleCountFlagBits m_samples = VK_SAMPLE_COUNT_1_BIT;

public:
  static VkSampleCountFlagBits getMaxUsableSampleCount(Context &context);

  void init(Context &context, Command &command, VkFormat colorFormat, uint32_t samples);
  void destroy() { m_image.destroy(); }

  bool enabled() const { return m_samples != VK_SAMPLE_COUNT_1_BIT; }

  VkImageView getView() const { return m_image.getView(); }
  VkImage getImage() const { return m_image.getImage(); }
  VkSampleCountFlagBits getSamples() const { return m_samples; }
};
