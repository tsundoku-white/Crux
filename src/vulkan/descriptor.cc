#include "descriptor.hh"
#include "src/vulkan/context.hh"
#include "src/vulkan/draw_call.hh"
#include <array>
#include <vulkan/vulkan_core.h>

void Descriptor::init(Context &context, uint32_t framesInFlight)
{
  m_device         = context.getDevice();
  m_allocator      = context.getAllocator();
  m_framesInFlight = framesInFlight;

  std::array<VkDescriptorSetLayoutBinding, 2> bindings {};
  bindings[0].binding            = 0;
  bindings[0].descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  bindings[0].descriptorCount    = 1;
  bindings[0].stageFlags         = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
  bindings[0].pImmutableSamplers = nullptr;

  bindings[1].binding            = 1;
  bindings[1].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  bindings[1].descriptorCount    = 1;
  bindings[1].stageFlags         = VK_SHADER_STAGE_VERTEX_BIT;
  bindings[1].pImmutableSamplers = nullptr;

  VkDescriptorSetLayoutCreateInfo layoutInfo {};
  layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = bindings.size();
  layoutInfo.pBindings    = bindings.data();

  if (vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_layout) != VK_SUCCESS)
    throw std::runtime_error("failed to create descriptor set layout!");

  VkDescriptorPoolSize sizes[2] {};
  sizes[0] = {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, framesInFlight};
  sizes[1] = {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, framesInFlight};

  VkDescriptorPoolCreateInfo ci {};
  ci.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  ci.maxSets       = framesInFlight;
  ci.poolSizeCount = 2;
  ci.pPoolSizes    = sizes;

  if (vkCreateDescriptorPool(m_device, &ci, nullptr, &m_pool) != VK_SUCCESS)
    throw std::runtime_error("failed to create descriptor pool!");

  std::vector<VkDescriptorSetLayout> layouts(m_framesInFlight, m_layout);
  VkDescriptorSetAllocateInfo        allocInfo {};
  allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool     = m_pool;
  allocInfo.descriptorSetCount = m_framesInFlight;
  allocInfo.pSetLayouts        = layouts.data();

  m_sets.resize(m_framesInFlight);
  if (vkAllocateDescriptorSets(m_device, &allocInfo, m_sets.data()) != VK_SUCCESS)
    throw std::runtime_error("failed to allocate descriptor sets!");

  m_buffers.resize(m_framesInFlight);
  m_allocs.resize(m_framesInFlight);
  m_mapped.resize(m_framesInFlight);

  for (uint32_t i = 0; i < m_framesInFlight; ++i)
  {
    VkBufferCreateInfo bufInfo {};
    bufInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufInfo.size        = sizeof(UBO);
    bufInfo.usage       = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocCreate {};
    allocCreate.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    allocCreate.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VmaAllocationInfo allocOut {};
    if (vmaCreateBuffer(m_allocator, &bufInfo, &allocCreate, &m_buffers[i], &m_allocs[i], &allocOut)
        != VK_SUCCESS)
      throw std::runtime_error("failed to create UBO buffer!");

    m_mapped[i] = allocOut.pMappedData;
  }

  // object SSBO, one per frame in flight
  m_ssboBuffers.resize(m_framesInFlight);
  m_ssboAllocs.resize(m_framesInFlight);
  m_ssboMapped.resize(m_framesInFlight);

  constexpr VkDeviceSize ssboSize = sizeof(ObjectData) * MAX_OBJECTS;
  for (uint32_t i = 0; i < m_framesInFlight; ++i)
  {
    VkBufferCreateInfo bufInfo {};
    bufInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufInfo.size        = ssboSize;
    bufInfo.usage       = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    bufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocCreate {};
    allocCreate.usage = VMA_MEMORY_USAGE_AUTO;
    allocCreate.flags =
      VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VmaAllocationInfo allocOut {};
    if (vmaCreateBuffer(m_allocator, &bufInfo, &allocCreate, &m_ssboBuffers[i], &m_ssboAllocs[i],
                        &allocOut)
        != VK_SUCCESS)
      throw std::runtime_error("failed to create object SSBO!");

    m_ssboMapped[i] = allocOut.pMappedData;
  }

  for (uint32_t i = 0; i < m_framesInFlight; ++i)
  {
    VkDescriptorBufferInfo bufferInfo {};
    bufferInfo.buffer = m_buffers[i];
    bufferInfo.offset = 0;
    bufferInfo.range  = sizeof(UBO);

    VkDescriptorBufferInfo ssboInfo {};
    ssboInfo.buffer = m_ssboBuffers[i];
    ssboInfo.offset = 0;
    ssboInfo.range  = ssboSize;

    std::array<VkWriteDescriptorSet, 2> writes {};
    writes[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].dstSet          = m_sets[i];
    writes[0].dstBinding      = 0;
    writes[0].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[0].descriptorCount = 1;
    writes[0].pBufferInfo     = &bufferInfo;

    writes[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[1].dstSet          = m_sets[i];
    writes[1].dstBinding      = 1;
    writes[1].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[1].descriptorCount = 1;
    writes[1].pBufferInfo     = &ssboInfo;

    vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(writes.size()), writes.data(), 0,
                           nullptr);
  }
}

void Descriptor::destroy()
{
  for (size_t i = 0; i < m_buffers.size(); ++i)
    vmaDestroyBuffer(m_allocator, m_buffers[i], m_allocs[i]);

  for (size_t i = 0; i < m_ssboBuffers.size(); ++i)
    vmaDestroyBuffer(m_allocator, m_ssboBuffers[i], m_ssboAllocs[i]);

  m_buffers.clear();
  m_allocs.clear();
  m_mapped.clear();
  m_ssboBuffers.clear();
  m_ssboAllocs.clear();
  m_ssboMapped.clear();

  if (m_pool != VK_NULL_HANDLE)
  {
    vkDestroyDescriptorPool(m_device, m_pool, nullptr);
    m_pool = VK_NULL_HANDLE;
  }

  if (m_layout != VK_NULL_HANDLE)
  {
    vkDestroyDescriptorSetLayout(m_device, m_layout, nullptr);
    m_layout = VK_NULL_HANDLE;
  }
}
