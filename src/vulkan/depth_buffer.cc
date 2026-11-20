#include "depth_buffer.hh"
#include "src/vulkan/context.hh"
#include "src/vulkan/image.hh"
#include "src/vulkan/msaa.hh"
#include <vulkan/vulkan_core.h>

void DepthBuffer::init(Context &context, Command &command, Msaa &msaa)
{
  m_format = VK_FORMAT_D32_SFLOAT;
  m_image.allocate(context, context.getExtent2D().width, context.getExtent2D().height, 1, m_format,
                   VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                   VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, msaa.getSamples());
  const VkImageAspectFlags aspect =
      VK_IMAGE_ASPECT_DEPTH_BIT | (hasStencil(m_format) ? VK_IMAGE_ASPECT_STENCIL_BIT : 0);

  VkCommandBuffer cmd = command.onceStart();
  m_image.transition(cmd, VK_IMAGE_LAYOUT_UNDEFINED,
                     VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_2_NONE, 0,
                     VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
                         VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                     VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                         VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                     0, 1, aspect);
  command.onceEnd(cmd);

  m_image.createView(aspect);
}

void DepthBuffer::destroy()
{
  m_format = VK_FORMAT_UNDEFINED;
  m_image.destroy();
}
