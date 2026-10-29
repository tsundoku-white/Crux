#pragma once

#include "src/core/pch.hh"

class Window;

class Context
{
private:
  vkb::Instance  m_vkbInst;
  vkb::Device    m_vkbDevice;
  vkb::Swapchain m_vkbSwapchain;

  GLFWwindow  *m_windowHandle   = nullptr;
  VkSurfaceKHR m_surface        = VK_NULL_HANDLE;
  VkDevice     m_device         = VK_NULL_HANDLE;
  VkQueue      m_graphicsQueue  = VK_NULL_HANDLE;
  VkQueue      m_presentQueue   = VK_NULL_HANDLE;
  uint32_t     m_graphicsFamily = 0;
  uint32_t     m_presentFamily  = 0;
  VmaAllocator m_allocator = VK_NULL_HANDLE;

  std::vector<VkImage>     m_images;
  std::vector<VkImageView> m_imageViews;

public:
  // -- functions ----------
  void init(Window &window, bool enabled_vsync);
  void destroy();
  void recreateSwapchain();

  void deviceWait() { vkDeviceWaitIdle(m_device); }

  // -- getters ----------
  VkDevice       getDevice()         const { return m_device; }
  VkQueue        getGraphicsQueue()  const { return m_graphicsQueue; }
  VkQueue        getPresentQueue()   const { return m_presentQueue; }
  uint32_t       getGraphicsFamily() const { return m_graphicsFamily; }
  uint32_t       getPresentFamily()  const { return m_presentFamily; }
  VkExtent2D     getExtent2D()       const { return m_vkbSwapchain.extent; }
  VkSwapchainKHR getSwapchain()      const { return m_vkbSwapchain.swapchain; }
  VkFormat       getImageFormat()    const { return m_vkbSwapchain.image_format; }
  uint32_t       getImageCount()     const { return m_vkbSwapchain.image_count; }
  VmaAllocator   getAllocator()      const { return m_allocator; }
  const std::vector<VkImage>&     getImages()     const { return m_images; }
  const std::vector<VkImageView>& getImageViews() const { return m_imageViews; }
};
