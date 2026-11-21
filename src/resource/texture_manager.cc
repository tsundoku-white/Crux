#include "texture_manager.hh"
#include "src/vulkan/command.hh"
#include "src/vulkan/context.hh"
#include <stb_image.h>
#include <vulkan/vulkan_core.h>

void TextureManager::init(Context &context, Command &command)
{
  // get device and store private and localy
  m_device = context.getDevice();

  // set the max count of textures
  VkDescriptorSetLayoutBinding binding {};
  binding.binding         = 0;
  binding.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  binding.descriptorCount = MAX_TEXTURES;
  binding.stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;

  // unused slots may stay unwritten
  VkDescriptorBindingFlags flags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;

  VkDescriptorSetLayoutBindingFlagsCreateInfo flagsInfo {};
  flagsInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
  flagsInfo.bindingCount  = 1;
  flagsInfo.pBindingFlags = &flags;

  VkDescriptorSetLayoutCreateInfo layoutInfo {};
  layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.pNext        = &flagsInfo;
  layoutInfo.bindingCount = 1;
  layoutInfo.pBindings    = &binding;
  if (vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_layout) != VK_SUCCESS)
    throw std::runtime_error("failed to create texture set layout!");

  VkDescriptorPoolSize size {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_TEXTURES};

  VkDescriptorPoolCreateInfo poolInfo {};
  poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.maxSets       = 1;
  poolInfo.poolSizeCount = 1;
  poolInfo.pPoolSizes    = &size;
  if (vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_pool) != VK_SUCCESS)
    throw std::runtime_error("failed to create texture descriptor pool!");

  // allocation
  VkDescriptorSetAllocateInfo allocInfo {};
  allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool     = m_pool;
  allocInfo.descriptorSetCount = 1;
  allocInfo.pSetLayouts        = &m_layout;
  if (vkAllocateDescriptorSets(m_device, &allocInfo, &m_set) != VK_SUCCESS)
    throw std::runtime_error("failed to allocate texture descriptor set!");

  // make sure the textures dont move.
  m_images.reserve(MAX_TEXTURES);

  // create a white texture.
  const uint8_t white[4] = {255, 255, 255, 255};
  add(context, command, "<white>", white, 1, 1, VK_FORMAT_R8G8B8A8_SRGB); // slot 0
  const uint8_t flat[4] = {128, 128, 255, 255};
  add(context, command, "<flat_normal>", flat, 1, 1, VK_FORMAT_R8G8B8A8_UNORM);
}

uint32_t
TextureManager::load(Context &context, Command &command, const std::string &path, TextureKind kind)
{
  // make sure the texture does not exist already.
  const bool        srgb     = kind == TextureKind::Color;
  const uint32_t    fallback = kind == TextureKind::Normal ? FLAT_NORMAL : 0u;
  const std::string key      = srgb ? path : path + "|linear";
  if (auto it = m_cache.find(path); it != m_cache.end())
    return it->second;

  // load the file pixel data.
  int      w = 0, h = 0, channels = 0;
  stbi_uc *pixels = stbi_load(path.c_str(), &w, &h, &channels, STBI_rgb_alpha);

  // if not find return this message.
  if (!pixels)
  {
    std::print("texture not found: {}\n", path);
    m_cache[key] = fallback;
    return fallback;
  }

  // create the resource for the image.
  uint32_t index = add(context, command, path, pixels, static_cast<uint32_t>(w),
                       static_cast<uint32_t>(h), VK_FORMAT_R8G8B8A8_SRGB);
  // free pixel data and return id.
  stbi_image_free(pixels);
  return index;
}

uint32_t TextureManager::loadHDR(Context &context, Command &command, const std::string &path)
{
  // make sure the texture does not exist already.
  if (auto it = m_cache.find(path); it != m_cache.end())
    return it->second;

  // load the file pixel data.
  int w = 0, h = 0, channels = 0;
  stbi_set_flip_vertically_on_load(true);
  float *pixels = stbi_loadf(path.c_str(), &w, &h, &channels, 4);
  stbi_set_flip_vertically_on_load(false);
  // if not find return this message.
  if (!pixels)
  {
    std::print("hdr not found, using white: {}\n", path);
    return 0;
  }

  // create the resource for the image.
  uint32_t index = add(context, command, path, pixels, static_cast<uint32_t>(w),
                       static_cast<uint32_t>(h), VK_FORMAT_R32G32B32A32_SFLOAT);
  // free pixel data and return id.
  stbi_image_free(pixels);
  return index;
}

uint32_t TextureManager::add(Context           &context,
                             Command           &command,
                             const std::string &key,
                             const void        *rgba,
                             uint32_t           w,
                             uint32_t           h,
                             VkFormat           format)
{
  // check if the texture thats being created does not exeed the max count.
  if (m_images.size() >= MAX_TEXTURES)
    throw std::runtime_error("texture array is full");

  // get index and create the resourcess of the image and cache it on ram.
  const uint32_t index = static_cast<uint32_t>(m_images.size());
  m_images.emplace_back();
  Image &img = m_images.back();
  img.createFromPixels(context, command, rgba, w, h, format);

  // set up the descriptor.
  VkDescriptorImageInfo info {img.getSampler(), img.getView(),
                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

  // write it to memory.
  VkWriteDescriptorSet write {};
  write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.dstSet          = m_set;
  write.dstBinding      = 0;
  write.dstArrayElement = index;
  write.descriptorCount = 1;
  write.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  write.pImageInfo      = &info;
  vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);

  // cache it to its index.
  m_cache[key] = index;
  return index;
}

// clear resources
void TextureManager::destroy()
{
  for (Image &img : m_images)
    img.destroy();
  m_images.clear();
  m_cache.clear();

  if (m_pool != VK_NULL_HANDLE)
    vkDestroyDescriptorPool(m_device, m_pool, nullptr);
  if (m_layout != VK_NULL_HANDLE)
    vkDestroyDescriptorSetLayout(m_device, m_layout, nullptr);

  m_pool   = VK_NULL_HANDLE;
  m_layout = VK_NULL_HANDLE;
  m_set    = VK_NULL_HANDLE;
}
