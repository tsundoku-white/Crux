#include "render.hh"
#include "src/platform/window.hh"
#include "src/vulkan/command.hh"
#include "src/vulkan/context.hh"
#include "src/vulkan/descriptor.hh"
#include "src/vulkan/pipeline.hh"

void Render::createSyncObjects(uint32_t imageCount)
{
  vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, nullptr);

  m_availableSemaphores.resize(m_frameInFlight); // per frame in flight
  m_inFlight.resize(m_frameInFlight);            // per frame in flight
  m_finishedSemaphores.resize(imageCount);       // per swapchain image

  VkSemaphoreCreateInfo semaphoreInfo {};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo {};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (size_t i = 0; i < m_frameInFlight; i++)
  {
    if (vkCreateSemaphore(m_device, &semaphoreInfo, nullptr,
                          &m_availableSemaphores[i]) != VK_SUCCESS ||
        vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlight[i]) != VK_SUCCESS)
      throw std::runtime_error("failed to create synchronization objects");
  }

  for (size_t i = 0; i < imageCount; i++)
  {
    if (vkCreateSemaphore(m_device, &semaphoreInfo, nullptr,
                          &m_finishedSemaphores[i]) != VK_SUCCESS)
      throw std::runtime_error("failed to create synchronization objects");
  }
}

void Render::init(Context &context, Command &command)
{
  m_device         = context.getDevice();
  m_swapchain      = context.getSwapchain();
  m_graphicsQueue  = context.getGraphicsQueue();
  m_presentQueue   = context.getPresentQueue();
  m_commandBuffers = command.getBuffers();
  createSyncObjects(context.getImageCount());
}

void Render::draw(Context &context, Window &window, Command *command, Pipeline &pipeline,
          Descriptor &descriptor, const Buffer &vertex, const Buffer &index, uint32_t index_count)
{
  vkWaitForFences(m_device, 1, &m_inFlight[m_currentFrame], VK_TRUE, UINT64_MAX);

  VkSwapchainKHR swapchain = context.getSwapchain();
  uint32_t       imageIndex;

  VkResult result = vkAcquireNextImageKHR(m_device, swapchain, UINT64_MAX,
                                          m_availableSemaphores[m_currentFrame], VK_NULL_HANDLE,
                                          &imageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR)
  {
    context.recreateSwapchain();
    return;
  }
  if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    throw std::runtime_error("Failed to acquire swapchain image");

  vkResetFences(m_device, 1, &m_inFlight[m_currentFrame]);

  vkResetCommandBuffer(m_commandBuffers[m_currentFrame], 0);

command->record(m_currentFrame, imageIndex, context, pipeline, descriptor,
                vertex, index, index_count);
  VkSemaphore          waitSemaphores[]   = {m_availableSemaphores[m_currentFrame]};
  VkPipelineStageFlags waitStages[]       = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  VkSemaphore          signalSemaphores[] = {m_finishedSemaphores[imageIndex]};

  VkSubmitInfo submitInfo {};
  submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.waitSemaphoreCount   = 1;
  submitInfo.pWaitSemaphores      = waitSemaphores;
  submitInfo.pWaitDstStageMask    = waitStages;
  submitInfo.commandBufferCount   = 1;
  submitInfo.pCommandBuffers      = &m_commandBuffers[m_currentFrame];
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores    = signalSemaphores;

  if (vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlight[m_currentFrame]) != VK_SUCCESS)
    throw std::runtime_error("failed to submit draw command buffer!");

  VkPresentInfoKHR presentInfo {};
  presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores    = signalSemaphores;
  VkSwapchainKHR swapChains[]    = {swapchain};
  presentInfo.swapchainCount     = 1;
  presentInfo.pSwapchains        = swapChains;
  presentInfo.pImageIndices      = &imageIndex;

  result = vkQueuePresentKHR(m_presentQueue, &presentInfo);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
      window.frameResized)
  {
    window.frameResized = false;
    context.recreateSwapchain();
  }
  else if (result != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to present swapchain image");
  }

  m_currentFrame = (m_currentFrame + 1) % m_frameInFlight;
}

void Render::destroy()
{
  for (size_t i = 0; i < m_frameInFlight; i++)
  {
    vkDestroySemaphore(m_device, m_availableSemaphores[i], nullptr);
    vkDestroyFence(m_device, m_inFlight[i], nullptr);
  }
  for (auto sem : m_finishedSemaphores)
    vkDestroySemaphore(m_device, sem, nullptr);
}
