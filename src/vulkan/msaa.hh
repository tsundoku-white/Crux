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
  // highest sample count supported by both color and depth framebuffers.
  static VkSampleCountFlagBits getMaxUsableSampleCount(Context &context);

  // colorFormat MUST match the swapchain format (the resolve target). Call again on resize.
  // samples is clamped to what the GPU supports; 1 means MSAA is off and no image is allocated.
  void init(Context &context, Command &command, VkFormat colorFormat, uint32_t samples);
  void destroy() { m_image.destroy(); }

  // true when rendering is multisampled (the color target has to be resolved to the swapchain).
  bool enabled() const { return m_samples != VK_SAMPLE_COUNT_1_BIT; }

  VkImageView           getView() const { return m_image.getView(); }
  VkImage               getImage() const { return m_image.getImage(); }
  VkSampleCountFlagBits getSamples() const { return m_samples; }
};
