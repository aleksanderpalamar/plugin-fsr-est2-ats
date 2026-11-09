#pragma once

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <optional>
#include <string>
#include <vector>

namespace spike {
struct VulkanDevice {
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physical = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    PFN_vkGetInstanceProcAddr instance_proc = nullptr;
    PFN_vkGetDeviceProcAddr device_proc = nullptr;
    uint32_t queue_family = 0;
    std::vector<std::string> extensions;
};

std::optional<VulkanDevice> create_vulkan_device();
}
