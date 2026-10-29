#pragma once

#include "src/core/pch.hh"
#include "src/vulkan/command.hh"
#include "src/vulkan/context.hh"
#include "src/vulkan/descriptor.hh"
#include "src/vulkan/pipeline.hh"

class Command;
class Context;
class Pipeline;

class Render
{
private:
  VkDevice                     m_device        = VK_NULL_HANDLE;
  VkSwapchainKHR               m_swapchain     = VK_NULL_HANDLE;
  VkQueue                      m_graphicsQueue = VK_NULL_HANDLE;
  VkQueue                      m_presentQueue  = VK_NULL_HANDLE;
  std::vector<VkCommandBuffer> m_commandBuffers;

  uint32_t m_frameInFlight = 3;

  std::vector<VkSemaphore> m_availableSemaphores = {};
  std::vector<VkSemaphore> m_finishedSemaphores  = {};
  std::vector<VkFence>     m_inFlight            = {};
  uint32_t                 m_currentFrame        = 0;

  void createSyncObjects(uint32_t imageCount);

public:
  void init(Context &context, Command &command);
  void destroy();
  void draw(Context &context, Window &window, Command *command, Pipeline &pipeline,
          Descriptor &descriptor, const Buffer &vertex, const Buffer &index, uint32_t index_count);
};
