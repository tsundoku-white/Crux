#pragma once
#include <vulkan/vulkan.h> // already in the PCH through GLFW

// Same macros VMA uses, so the typedefs always match
VK_DEFINE_HANDLE(VmaAllocator)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VmaAllocation)

typedef enum VmaMemoryUsage
{
} VmaMemoryUsage;
using VmaAllocationCreateFlags = VkFlags;
