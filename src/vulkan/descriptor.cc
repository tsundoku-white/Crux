#include "descriptor.hh"
#include "src/vulkan/context.hh"

void Descriptor::init(Context &context, uint32_t framesInFlight)
{
  m_device         = context.getDevice();
  m_allocator      = context.getAllocator();
  m_framesInFlight = framesInFlight;

  VkDescriptorSetLayoutBinding uboBinding {};
  uboBinding.binding            = 0;
  uboBinding.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  uboBinding.descriptorCount    = 1;
  uboBinding.stageFlags         = VK_SHADER_STAGE_VERTEX_BIT; // add FRAGMENT if needed
  uboBinding.pImmutableSamplers = nullptr;

  VkDescriptorSetLayoutCreateInfo layoutInfo {};
  layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = 1;
  layoutInfo.pBindings    = &uboBinding;

  if (vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_layout) != VK_SUCCESS)
    throw std::runtime_error("failed to create descriptor set layout!");

  VkDescriptorPoolSize poolSize {};
  poolSize.type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSize.descriptorCount = m_framesInFlight;

  VkDescriptorPoolCreateInfo poolInfo {};
  poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = 1;
  poolInfo.pPoolSizes    = &poolSize;
  poolInfo.maxSets       = m_framesInFlight;

  if (vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_pool) != VK_SUCCESS)
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
    if (vmaCreateBuffer(m_allocator, &bufInfo, &allocCreate,
                        &m_buffers[i], &m_allocs[i], &allocOut) != VK_SUCCESS)
      throw std::runtime_error("failed to create UBO buffer!");

    m_mapped[i] = allocOut.pMappedData;
  }

  for (uint32_t i = 0; i < m_framesInFlight; ++i)
  {
    VkDescriptorBufferInfo bufferInfo {};
    bufferInfo.buffer = m_buffers[i];
    bufferInfo.offset = 0;
    bufferInfo.range  = sizeof(UBO);

    VkWriteDescriptorSet write {};
    write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet          = m_sets[i];
    write.dstBinding      = 0;
    write.dstArrayElement = 0;
    write.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write.descriptorCount = 1;
    write.pBufferInfo     = &bufferInfo;

    vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);
  }
}

void Descriptor::destroy()
{
  for (size_t i = 0; i < m_buffers.size(); ++i)
    vmaDestroyBuffer(m_allocator, m_buffers[i], m_allocs[i]);

  m_buffers.clear();
  m_allocs.clear();
  m_mapped.clear();

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
