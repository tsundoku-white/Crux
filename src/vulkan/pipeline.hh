#pragma once

#include "src/core/pch.hh"
#include "src/vulkan/descriptor.hh"

class Context;

class Pipeline
{
private:
  VkDevice         m_device   = VK_NULL_HANDLE;
  VkPipelineLayout m_layout   = VK_NULL_HANDLE;
  VkPipeline       m_pipeline = VK_NULL_HANDLE;

  // -- helper functions ----------
  std::vector<char> readFile(const char *path);
  VkShaderModule    createShaderModule(const std::vector<char> &code);

public:
  // -- functions ----------
  void init(Context &context, Descriptor &descriptor);
  void destroy();

  // -- getters ----------
  VkPipelineLayout getLayout() const { return m_layout; }

  VkPipeline getPipeline() const { return m_pipeline; }
};
