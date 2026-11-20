#pragma once

#include "src/core/pch.hh"
#include "src/vulkan/depth_buffer.hh"
#include "src/vulkan/descriptor.hh"
#include "src/vulkan/msaa.hh"
#include <vulkan/vulkan_core.h>

class Context;

class Pipeline
{
private:
  VkDevice         m_device         = VK_NULL_HANDLE;
  VkPipelineLayout m_layout         = VK_NULL_HANDLE;
  VkPipeline       m_pipeline       = VK_NULL_HANDLE;
  VkPipeline       m_skyboxPipeline = VK_NULL_HANDLE;
  // -- helper functions ----------
  std::vector<char> readFile(const char *path);
  VkShaderModule    createShaderModule(const std::vector<char> &code);

public:
  // -- functions ----------
  // texLayout = TextureManager::layout() (set 1). Not owned by the pipeline.
  void init(Context &context, Descriptor &descriptor, VkDescriptorSetLayout texLayout, Msaa &msaa);
  void destroy();

  // -- getters ----------
  VkPipelineLayout getLayout() const { return m_layout; }
  VkPipeline       getPipeline() const { return m_pipeline; }
  VkPipeline       getSkyboxPipeline() const { return m_skyboxPipeline; }
};
