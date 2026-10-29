#include "context.hh"
#include "src/platform/window.hh"

void Context::init(Window &window, bool enabled_vsync)
{
  // getting the window handle.
  m_windowHandle = window.getHandle();

  // creating the instance builder.
  vkb::InstanceBuilder builder;
  auto                 inst_ret = builder.set_app_name("Example Vulkan Application")
                                      .request_validation_layers()
                                      .use_default_debug_messenger()
                                      .require_api_version(1, 4, 0)
                                      .build();

  // checking if the instance is valid.
  if (!inst_ret)
  {
    throw std::runtime_error("Failed to create Vulkan instance.\n");
  }

  // storing local copy.
  m_vkbInst = inst_ret.value();

  // creating glfw suface.
  VkResult glfw_result = glfwCreateWindowSurface(m_vkbInst, window.getHandle(),
                                                 nullptr, &m_surface);
  // checking if surface is valid.
  if (glfw_result != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to select create window surface.\n");
  }

  // device settings
  VkPhysicalDeviceVulkan12Features f12 {};
  f12.bufferDeviceAddress = VK_TRUE;

  VkPhysicalDeviceVulkan13Features f13 {};
  f13.dynamicRendering = VK_TRUE;
  f13.synchronization2 = VK_TRUE;

  // creating physical device builder.
  vkb::PhysicalDeviceSelector selector {m_vkbInst};
  auto                        phys_ret =
      selector.set_surface(m_surface)
          .set_required_features_12(f12)
          .set_required_features_13(f13)
          .select();
  if (!phys_ret)
  {
    throw std::runtime_error("Failed to select Vulkan Physical Device.\n");
    if (phys_ret.error() == vkb::PhysicalDeviceError::no_suitable_device)
    {
      const auto &detailed_reasons = phys_ret.detailed_failure_reasons();
      if (!detailed_reasons.empty())
      {
        throw std::runtime_error("GPU Selection failure reasons:\n");
        for (const std::string &reason : detailed_reasons)
        {
          std::print("{}\n", reason.c_str());
        }
      }
    }
  }

  // creating device builder
  vkb::DeviceBuilder device_builder {phys_ret.value()};

  // checking if device is valid.
  auto dev_ret = device_builder.build();
  if (!dev_ret)
  {
    throw std::runtime_error("Failed to create Vulkan device.\n");
  }
  // storing local ref.
  m_vkbDevice = dev_ret.value();

  // storing local ref.
  m_device = m_vkbDevice.device;

  auto gq = m_vkbDevice.get_queue(vkb::QueueType::graphics);
  auto pq = m_vkbDevice.get_queue(vkb::QueueType::present);
  if (!gq || !pq)
    throw std::runtime_error("failed to get queues: " + gq.error().message());

  m_graphicsQueue = gq.value();
  m_presentQueue  = pq.value();
  m_graphicsFamily =
      m_vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
  m_presentFamily =
      m_vkbDevice.get_queue_index(vkb::QueueType::present).value();

  // creating swapchain.
  vkb::SwapchainBuilder swapchain_builder {m_vkbDevice};
  auto                  swap_ret = swapchain_builder.build();

  // enabling vsync.
  if (enabled_vsync)
  {
    swapchain_builder.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR);
  }
  else
  {
    swapchain_builder.set_desired_present_mode(VK_PRESENT_MODE_IMMEDIATE_KHR);
  }

  // checking if valid.
  if (!swap_ret)
  {
    std::print("err msg: {}\n", swap_ret.error().message());
  }

  // storing local ref.
  m_vkbSwapchain = swap_ret.value();
  m_images       = m_vkbSwapchain.get_images().value();
  m_imageViews   = m_vkbSwapchain.get_image_views().value();

  VmaAllocatorCreateInfo allocatorInfo = {};
  allocatorInfo.physicalDevice         = m_vkbDevice.physical_device;
  allocatorInfo.device                 = m_device;
  allocatorInfo.instance               = m_vkbInst.instance;
  allocatorInfo.vulkanApiVersion       = VK_API_VERSION_1_3; // match your device features
  allocatorInfo.flags                  = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

  if (vmaCreateAllocator(&allocatorInfo, &m_allocator) != VK_SUCCESS)
    throw std::runtime_error("failed to create VMA allocator!");
  std::print("pass: context\n");
}

void Context::recreateSwapchain()
{
  // temp width and height of window size.
  int width = 0, height = 0;
  glfwGetFramebufferSize(m_windowHandle, &width, &height);
  while (width == 0 || height == 0)
  { // paused while minimized
    glfwGetFramebufferSize(m_windowHandle, &width, &height);
    glfwWaitEvents();
  }

  // wait for all calls to finish.
  deviceWait();

  // build new swapchain.
  vkb::SwapchainBuilder builder {m_vkbDevice};
  auto                  ret = builder
                                  .set_old_swapchain(m_vkbSwapchain)
                                  .set_desired_extent(width, height)
                                  .build();
  if (!ret)
  {
    throw std::runtime_error("Failed to recreate swapchain: " +
                             ret.error().message());
  }

  // destroying old swapchain.
  m_vkbSwapchain.destroy_image_views(m_imageViews);
  vkb::destroy_swapchain(m_vkbSwapchain);

  // update all local refs that need to updated
  m_vkbSwapchain = ret.value();
  m_images       = m_vkbSwapchain.get_images().value();
  m_imageViews   = m_vkbSwapchain.get_image_views().value();
}

void Context::destroy()
{
  vmaDestroyAllocator(m_allocator);
  m_vkbSwapchain.destroy_image_views(m_imageViews);
  vkb::destroy_swapchain(m_vkbSwapchain);
  vkb::destroy_device(m_vkbDevice);
  vkb::destroy_surface(m_vkbInst, m_surface);
  vkb::destroy_instance(m_vkbInst);
}
