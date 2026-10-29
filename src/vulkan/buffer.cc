#include "buffer.hh"
#include "src/core/pch.hh"
#include <vulkan/vulkan_core.h>

void Buffer::create_buffer(Context &context, VkDeviceSize size,
                   VkBufferUsageFlags usage, VmaMemoryUsage memory_usage,
                   VmaAllocationCreateFlags alloc_flags)
{
  if (size == 0)
    throw std::runtime_error("create_buffer: attempted to create a zero-size buffer");

  VkBufferCreateInfo bufferInfo {};
  bufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size        = size;
  bufferInfo.usage       = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VmaAllocationCreateInfo allocInfo {};
  allocInfo.usage = memory_usage;
  allocInfo.flags = alloc_flags;

  VmaAllocationInfo nAllocInfo {};
  if (vmaCreateBuffer(m_allocator, &bufferInfo, &allocInfo,
                      &m_buffer, &m_alloc, &nAllocInfo) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create buffer");
  }

  m_size   = size;
  m_mapped = nAllocInfo.pMappedData;

  if (usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
  {
    VkBufferDeviceAddressInfo addInfo {};
    addInfo.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    addInfo.buffer = m_buffer;
    m_address      = vkGetBufferDeviceAddress(context.getDevice(), &addInfo);
  }
}

void Buffer::copy(Context &context, Command &command, VkBuffer src, VkBuffer dst, VkDeviceSize size)
{
  VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool        = command.getPool();
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(context.getDevice(), &allocInfo, &cmd);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(cmd, src, dst, 1, &copyRegion);

    vkEndCommandBuffer(cmd);

    VkSubmitInfo submitInfo{};
    submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &cmd;

    vkQueueSubmit(context.getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(context.getGraphicsQueue()); 

    vkFreeCommandBuffers(context.getDevice(), command.getPool(), 1, &cmd);
}

void Buffer::createStatic(Context &context, Command &command, BufferKind kind, const void *data, VkDeviceSize size, bool device_address)
{
  m_allocator = context.getAllocator();

  Buffer staging {};
  staging.m_allocator = m_allocator;

  staging.create_buffer(context, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                        VMA_MEMORY_USAGE_AUTO,
                        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
  std::memcpy(staging.m_mapped, data, size);

  VkBufferUsageFlags usage = std::to_underlying(kind) | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  if (device_address)
    usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

  create_buffer(context, size, usage, VMA_MEMORY_USAGE_AUTO, 0);
  copy(context, command, staging.m_buffer, m_buffer, size);
  staging.destroy();
}

void Buffer::createDynamic(Context &context, BufferKind kind, VkDeviceSize size, bool device_address)
{
  m_allocator = context.getAllocator();

    VkBufferUsageFlags usage = std::to_underlying(kind);
    if (device_address)
        usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

    create_buffer(context, size, usage,
        VMA_MEMORY_USAGE_AUTO,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
        | VMA_ALLOCATION_CREATE_MAPPED_BIT);
}

void Buffer::update(const void *data, VkDeviceSize size, VkDeviceSize offset)
{
  std::memcpy(m_mapped, data, size);
  vmaFlushAllocation(m_allocator, m_alloc, offset, size);
}

void Buffer::destroy()
{
  if (m_buffer != VK_NULL_HANDLE)
    vmaDestroyBuffer(m_allocator, m_buffer, m_alloc);
}
