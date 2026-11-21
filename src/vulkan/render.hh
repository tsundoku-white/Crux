#pragma once

#include "src/core/pch.hh"
#include "src/resource/resource_manager.hh"
#include "src/vulkan/depth_buffer.hh"
#include "src/vulkan/msaa.hh"
#include "src/resource/texture_manager.hh"

class Command;
class Context;
class Pipeline;
class Window;
class Descriptor;

class Render
{
private:
  VkDevice                     m_device              = VK_NULL_HANDLE;
  VkSwapchainKHR               m_swapchain           = VK_NULL_HANDLE;
  VkQueue                      m_graphicsQueue       = VK_NULL_HANDLE;
  VkQueue                      m_presentQueue        = VK_NULL_HANDLE;
  std::vector<VkCommandBuffer> m_commandBuffers      = {};
  uint32_t                     m_frameInFlight       = 3;
  std::vector<VkSemaphore>     m_availableSemaphores = {};
  std::vector<VkSemaphore>     m_finishedSemaphores  = {};
  std::vector<VkFence>         m_inFlight            = {};
  uint32_t                     m_currentFrame        = 0;

  DepthBuffer *m_depthBuffer;
  Msaa        *m_msaa;

  void createSyncObjects(uint32_t imageCount);
  void recreate(Context &context, Command &command, ResourceManager &resourceManger);

public:
  void init(Context &context, Command &command, DepthBuffer &depth_buffer, Msaa &msaa);
  void destroy();
  void draw(Context         &context,
            Window          &window,
            Command         *command,
            Pipeline        &pipeline,
            Descriptor      &descriptor,
            TextureManager  &textures,
            ResourceManager &resourceManager);

  float deltaTime = 0;
};
