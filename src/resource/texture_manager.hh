#pragma once

#include "src/core/pch.hh"
#include "src/vulkan/image.hh"
#include <unordered_map>
#include <vulkan/vulkan_core.h>

// forward declirations
class Context;
class Command;

class TextureManager
{
private:
  // helper
  uint32_t add(Context &context, Command &command, const std::string &key, const void *rgba,
               uint32_t w, uint32_t h, VkFormat format);

  // private variables
  VkDevice              m_device = VK_NULL_HANDLE;
  VkDescriptorSetLayout m_layout = VK_NULL_HANDLE;
  VkDescriptorPool      m_pool   = VK_NULL_HANDLE;
  VkDescriptorSet       m_set    = VK_NULL_HANDLE;

  // cashed textures
  std::vector<Image>                        m_images;
  std::unordered_map<std::string, uint32_t> m_cache;

public:
  // static const
  static constexpr uint32_t MAX_TEXTURES = 1024;
  // -- functions ----------
  void     init(Context &context, Command &command);
  void     destroy();
  uint32_t load(Context &context, Command &command, const std::string &path);
  uint32_t loadHDR(Context &context, Command &command, const std::string &path);

  // -- getters ----------
  VkDescriptorSetLayout getLayout() const { return m_layout; }
  VkDescriptorSet       getSet() const { return m_set; }
};
