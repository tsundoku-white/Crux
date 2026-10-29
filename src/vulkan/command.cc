#include "command.hh"
#include "src/vulkan/context.hh"
#include "src/vulkan/descriptor.hh"
#include "src/vulkan/buffer.hh"
#include "src/vulkan/pipeline.hh"
#include <vulkan/vulkan_core.h>

// -- functions ---------
void Command::init(Context &context, uint32_t framesInFlight)
{
  // allocate device in local ref.
  m_device = context.getDevice();

  VkCommandPoolCreateInfo poolInfo {};
  poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  poolInfo.queueFamilyIndex = context.getGraphicsFamily();

  if (vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_pool) !=
      VK_SUCCESS)
  {
    throw std::runtime_error("failed to create command pool!");
  }

  // resize buffer.
  m_buffers.resize(framesInFlight);

  VkCommandBufferAllocateInfo allocInfo {};
  allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool        = m_pool;
  allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = (uint32_t)m_buffers.size();

  if (vkAllocateCommandBuffers(m_device, &allocInfo, m_buffers.data()) !=
      VK_SUCCESS)
  {
    throw std::runtime_error("failed to allocate command buffers!");
  }
}

void Command::destroy()
{
  if (m_pool != VK_NULL_HANDLE)
    vkDestroyCommandPool(m_device, m_pool, nullptr);
}

static void transition(VkCommandBuffer cmd, VkImage image,
                       VkImageLayout from, VkImageLayout to,
                       VkPipelineStageFlags2 srcStage, VkAccessFlags2 srcAccess,
                       VkPipelineStageFlags2 dstStage, VkAccessFlags2 dstAccess)
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
  b.image                                       = image;
  b.subresourceRange                            = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

  VkDependencyInfo dep {};
  dep.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
  dep.imageMemoryBarrierCount = 1;
  dep.pImageMemoryBarriers    = &b;
  vkCmdPipelineBarrier2(cmd, &dep);
}

void Command::record(uint32_t currentFrame, uint32_t imageIndex,
            Context &context, Pipeline &pipeline, Descriptor &descriptor,
            const Buffer &vertex, const Buffer &index, uint32_t index_count)
{
  VkCommandBuffer cmd    = m_buffers[currentFrame];
  VkImage         image  = context.getImages()[imageIndex];
  VkImageView     view   = context.getImageViews()[imageIndex];
  VkExtent2D      extent = context.getExtent2D();

  VkCommandBufferBeginInfo begin {};
  begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  if (vkBeginCommandBuffer(cmd, &begin) != VK_SUCCESS)
    throw std::runtime_error("failed to begin command buffer!");

  // UNDEFINED -> COLOR_ATTACHMENT_OPTIMAL
  transition(cmd, image, VK_IMAGE_LAYOUT_UNDEFINED,
             VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
             VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, 0,
             VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
             VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);

  VkRenderingAttachmentInfo color {};
  color.sType            = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
  color.imageView        = view;
  color.imageLayout      = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  color.loadOp           = VK_ATTACHMENT_LOAD_OP_CLEAR;
  color.storeOp          = VK_ATTACHMENT_STORE_OP_STORE;
  color.clearValue.color = {{0.0f, 0.0f, 0.0f, 1.0f}};

  VkRenderingInfo ri {};
  ri.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO;
  ri.renderArea           = {{0, 0}, extent};
  ri.layerCount           = 1;
  ri.colorAttachmentCount = 1;
  ri.pColorAttachments    = &color;

  vkCmdBeginRendering(cmd, &ri);
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.getPipeline());
  VkBuffer     vb[] = {vertex.handle()};
  VkDeviceSize off[]       = {0};
  vkCmdBindVertexBuffers(cmd, 0, 1, vb, off);
vkCmdBindIndexBuffer(cmd, index.handle(), 0, VK_INDEX_TYPE_UINT32);
  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.getLayout(), 0, 1, &descriptor.getSets()[currentFrame], 0, nullptr);
  VkViewport vp {0.0f, 0.0f, (float)extent.width, (float)extent.height,
                 0.0f, 1.0f};
  VkRect2D   sc {{0, 0}, extent};
  vkCmdSetViewport(cmd, 0, 1, &vp);
  vkCmdSetScissor(cmd, 0, 1, &sc);
  vkCmdDrawIndexed(cmd, index_count, 1, 0, 0, 0);
  vkCmdEndRendering(cmd);

  transition(cmd, image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
             VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
             VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
             VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
             VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, 0);

  if (vkEndCommandBuffer(cmd) != VK_SUCCESS)
    throw std::runtime_error("failed to end command buffer!");
}
