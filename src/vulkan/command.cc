#include "command.hh"
#include "src/vulkan/context.hh"
#include "src/vulkan/depth_buffer.hh"
#include "src/vulkan/descriptor.hh"
#include "src/vulkan/buffer.hh"
#include "src/vulkan/msaa.hh"
#include "src/vulkan/pipeline.hh"
#include <vulkan/vulkan_core.h>

// -- functions ---------
void Command::init(Context &context, uint32_t framesInFlight)
{
  // allocate device in local ref.
  m_device = context.getDevice();
  m_queue  = context.getGraphicsQueue();

  VkCommandPoolCreateInfo poolInfo {};
  poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  poolInfo.queueFamilyIndex = context.getGraphicsFamily();

  if (vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_pool) != VK_SUCCESS)
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

  if (vkAllocateCommandBuffers(m_device, &allocInfo, m_buffers.data()) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to allocate command buffers!");
  }
}

void Command::destroy()
{
  if (m_pool != VK_NULL_HANDLE)
    vkDestroyCommandPool(m_device, m_pool, nullptr);
}

static void transition(VkCommandBuffer       cmd,
                       VkImage               image,
                       VkImageLayout         from,
                       VkImageLayout         to,
                       VkPipelineStageFlags2 srcStage,
                       VkAccessFlags2        srcAccess,
                       VkPipelineStageFlags2 dstStage,
                       VkAccessFlags2        dstAccess,
                       VkImageAspectFlags    aspect = VK_IMAGE_ASPECT_COLOR_BIT)
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
  b.subresourceRange                            = {aspect, 0, 1, 0, 1};

  VkDependencyInfo dep {};
  dep.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
  dep.imageMemoryBarrierCount = 1;
  dep.pImageMemoryBarriers    = &b;
  vkCmdPipelineBarrier2(cmd, &dep);
}

VkCommandBuffer Command::onceStart()
{
  VkCommandBufferAllocateInfo allocInfo {};
  allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool        = m_pool;
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);

  VkCommandBufferBeginInfo beginInfo {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(commandBuffer, &beginInfo);
  return commandBuffer;
}

void Command::onceEnd(VkCommandBuffer commandBuffer)
{
  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo {};
  submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers    = &commandBuffer;

  vkQueueSubmit(m_queue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(m_queue);

  vkFreeCommandBuffers(m_device, m_pool, 1, &commandBuffer);
}

void Command::record(uint32_t                       currentFrame,
                     uint32_t                       imageIndex,
                     Context                       &context,
                     Pipeline                      &pipeline,
                     Descriptor                    &descriptor,
                     VkDescriptorSet                textureSet,
                     DepthBuffer                   &depthBuffer,
                     Msaa                          &msaa,
                     std::span<const DrawCall>      drawCalls,
                     const std::optional<DrawCall> &skyDraw)
{
  VkCommandBuffer cmd    = m_buffers[currentFrame];
  VkImage         image  = context.getImages()[imageIndex];
  VkExtent2D      extent = context.getExtent2D();

  VkCommandBufferBeginInfo begin {};
  begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  if (vkBeginCommandBuffer(cmd, &begin) != VK_SUCCESS)
    throw std::runtime_error("failed to begin command buffer!");

  transition(cmd, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
             VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, 0,
             VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
             VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);

  VkImageView swapView = context.getImageViews()[imageIndex];

  // the msaa color image and the depth image are shared by every frame in flight, so make this
  // frame wait for the previous frame's writes before it clears them again.
  if (msaa.enabled())
    transition(
      cmd, msaa.getImage(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
      VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
      VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);

  transition(
    cmd, depthBuffer.getImage(), VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
    VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
    VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
    VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
    VK_IMAGE_ASPECT_DEPTH_BIT);

  VkRenderingAttachmentInfo color {};
  color.sType            = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
  color.loadOp           = VK_ATTACHMENT_LOAD_OP_CLEAR;
  color.clearValue.color = {{0.1f, 0.2f, 0.4f, 1.0f}};
  if (msaa.enabled())
  {
    // draw into the multisampled image, resolve it into the swapchain image.
    color.imageView          = msaa.getView();
    color.imageLayout        = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color.resolveMode        = VK_RESOLVE_MODE_AVERAGE_BIT;
    color.resolveImageView   = swapView;
    color.resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color.storeOp            = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  }
  else
  {
    // msaa off: draw straight into the swapchain image.
    color.imageView   = swapView;
    color.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color.resolveMode = VK_RESOLVE_MODE_NONE;
    color.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;
  }

  VkRenderingAttachmentInfo depth {};
  depth.sType                   = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
  depth.imageView               = depthBuffer.getView();
  depth.imageLayout             = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  depth.loadOp                  = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depth.storeOp                 = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depth.clearValue.depthStencil = {1.0f, 0};

  VkRenderingInfo ri {};
  ri.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO;
  ri.renderArea           = {{0, 0}, extent};
  ri.layerCount           = 1;
  ri.colorAttachmentCount = 1;
  ri.pColorAttachments    = &color;
  ri.pDepthAttachment     = &depth;

  vkCmdBeginRendering(cmd, &ri);
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.getPipeline());
  VkDescriptorSet sets[] = {descriptor.getSets()[currentFrame], textureSet};
  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.getLayout(), 0, 2, sets, 0,
                          nullptr);
  VkViewport vp {0.0f, 0.0f, (float)extent.width, (float)extent.height, 0.0f, 1.0f};
  VkRect2D   sc {{0, 0}, extent};

  vkCmdSetViewport(cmd, 0, 1, &vp);
  vkCmdSetScissor(cmd, 0, 1, &sc);

  for (const DrawCall &d : drawCalls)
  {
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &d.vertexBuffer, &offset);
    vkCmdBindIndexBuffer(cmd, d.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(cmd, d.indexCount, d.instanceCount, d.firstIndex, d.vertexOffset,
                     d.firstInstance);
  }

  if (skyDraw)
  {
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.getSkyboxPipeline());

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &skyDraw->vertexBuffer, &offset);
    vkCmdBindIndexBuffer(cmd, skyDraw->indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(cmd, skyDraw->indexCount, 1, 0, 0, skyDraw->firstInstance);
  }

  vkCmdEndRendering(cmd);

  transition(cmd, image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
             VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
             VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, 0);

  if (vkEndCommandBuffer(cmd) != VK_SUCCESS)
    throw std::runtime_error("failed to end command buffer!");
}
