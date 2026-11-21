#include "msaa.hh"
#include "src/vulkan/command.hh"
#include "src/vulkan/context.hh"
#include <algorithm>
#include <bit>

VkSampleCountFlagBits Msaa::getMaxUsableSampleCount(Context &context)
{
  VkPhysicalDeviceProperties props;
  vkGetPhysicalDeviceProperties(context.getPhysicalDevice(), &props);

  const VkSampleCountFlags counts =
    props.limits.framebufferColorSampleCounts & props.limits.framebufferDepthSampleCounts;

  return static_cast<VkSampleCountFlagBits>(std::bit_floor(counts));
}

void Msaa::init(Context &context, Command &command, VkFormat colorFormat, uint32_t samples)
{
  const uint32_t maxSamples = getMaxUsableSampleCount(context);
  m_samples =
    static_cast<VkSampleCountFlagBits>(std::bit_floor(std::clamp(samples, 1u, maxSamples)));

  m_image.destroy();

  if (m_samples == VK_SAMPLE_COUNT_1_BIT)
    return;

  const VkExtent2D extent = context.getExtent2D();

  m_image.allocate(context, extent.width, extent.height, 1, colorFormat,
                   VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, 0, m_samples);
  m_image.createView(VK_IMAGE_ASPECT_COLOR_BIT);
  VkCommandBuffer cmd = command.onceStart();
  m_image.transition(cmd, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                     VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE,
                     VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                     VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, 0, 1);
  command.onceEnd(cmd);
}
