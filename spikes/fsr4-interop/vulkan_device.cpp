#include "vulkan_device.hpp"
#include "report.hpp"

#include <windows.h>
#include <algorithm>

namespace spike {
namespace {
constexpr const char* wanted_extensions[]{
    "VK_KHR_cooperative_matrix", "VK_EXT_shader_float8", "VK_KHR_shader_float16_int8",
    "VK_EXT_subgroup_size_control", "VK_EXT_descriptor_indexing", "VK_KHR_synchronization2",
    "VK_KHR_shader_integer_dot_product", "VK_AMD_buffer_marker", "VK_AMD_device_coherent_memory",
    "VK_KHR_external_memory_win32", "VK_KHR_external_semaphore_win32",
};

template <typename Structure>
Structure chained(VkStructureType type, void* next = nullptr) {
    Structure structure{};
    structure.sType = type;
    structure.pNext = next;
    return structure;
}

template <typename Function>
Function instance_function(const VulkanDevice& vulkan, const char* name) {
    return reinterpret_cast<Function>(vulkan.instance_proc(vulkan.instance, name));
}

bool create_instance(VulkanDevice& vulkan) {
    HMODULE module = LoadLibraryW(L"vulkan-1.dll");
    if (!report("vk.loader", module ? S_OK : E_FAIL)) return false;
    vulkan.instance_proc = reinterpret_cast<PFN_vkGetInstanceProcAddr>(GetProcAddress(module, "vkGetInstanceProcAddr"));
    auto create = reinterpret_cast<PFN_vkCreateInstance>(vulkan.instance_proc(nullptr, "vkCreateInstance"));
    VkApplicationInfo application{VK_STRUCTURE_TYPE_APPLICATION_INFO, nullptr, "neuralfx-fsr4-probe", 1, "neuralfx", 1,
        VK_API_VERSION_1_3};
    VkInstanceCreateInfo info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, nullptr, 0, &application, 0, nullptr, 0, nullptr};
    VkResult result = create ? create(&info, nullptr, &vulkan.instance) : VK_ERROR_INITIALIZATION_FAILED;
    return report("vk.instance", result == VK_SUCCESS ? S_OK : E_FAIL, "vk_result=" + std::to_string(result));
}

bool pick_device(VulkanDevice& vulkan) {
    auto enumerate = instance_function<PFN_vkEnumeratePhysicalDevices>(vulkan, "vkEnumeratePhysicalDevices");
    auto properties = instance_function<PFN_vkGetPhysicalDeviceProperties2>(vulkan, "vkGetPhysicalDeviceProperties2");
    uint32_t count = 0;
    enumerate(vulkan.instance, &count, nullptr);
    std::vector<VkPhysicalDevice> devices(count);
    enumerate(vulkan.instance, &count, devices.data());
    for (auto device : devices) {
        auto driver = chained<VkPhysicalDeviceDriverProperties>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES);
        auto description = chained<VkPhysicalDeviceProperties2>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2, &driver);
        properties(device, &description);
        if (description.properties.vendorID != 0x1002) continue;
        vulkan.physical = device;
        return report("vk.physical", S_OK, std::string("name=") + description.properties.deviceName +
            " driver=" + driver.driverName + " info=" + driver.driverInfo + " driver_id=" +
            std::to_string(driver.driverID) + " api=0x" + std::to_string(description.properties.apiVersion));
    }
    return report("vk.physical", E_FAIL, "no AMD device");
}

std::vector<std::string> supported_extensions(const VulkanDevice& vulkan) {
    auto enumerate = instance_function<PFN_vkEnumerateDeviceExtensionProperties>(vulkan,
        "vkEnumerateDeviceExtensionProperties");
    uint32_t count = 0;
    enumerate(vulkan.physical, nullptr, &count, nullptr);
    std::vector<VkExtensionProperties> available(count);
    enumerate(vulkan.physical, nullptr, &count, available.data());
    std::vector<std::string> chosen;
    for (const char* wanted : wanted_extensions) {
        bool present = std::any_of(available.begin(), available.end(),
            [&](const VkExtensionProperties& extension) { return std::string(extension.extensionName) == wanted; });
        std::printf("vk.extension name=%s supported=%d\n", wanted, present ? 1 : 0);
        if (present) chosen.emplace_back(wanted);
    }
    return chosen;
}

bool create_logical(VulkanDevice& vulkan) {
    auto features = instance_function<PFN_vkGetPhysicalDeviceFeatures2>(vulkan, "vkGetPhysicalDeviceFeatures2");
    auto create = instance_function<PFN_vkCreateDevice>(vulkan, "vkCreateDevice");
    auto v13 = chained<VkPhysicalDeviceVulkan13Features>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES);
    auto v12 = chained<VkPhysicalDeviceVulkan12Features>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES, &v13);
    auto v11 = chained<VkPhysicalDeviceVulkan11Features>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES, &v12);
    auto all = chained<VkPhysicalDeviceFeatures2>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, &v11);
    features(vulkan.physical, &all);
    std::printf("vk.features shaderInt8=%u shaderFloat16=%u integerDotProduct=%u subgroupSizeControl=%u\n",
        v12.shaderInt8, v12.shaderFloat16, v13.shaderIntegerDotProduct, v13.subgroupSizeControl);
    vulkan.extensions = supported_extensions(vulkan);
    std::vector<const char*> names;
    for (const auto& name : vulkan.extensions) names.push_back(name.c_str());
    float priority = 1.0f;
    VkDeviceQueueCreateInfo queue{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, nullptr, 0, vulkan.queue_family, 1, &priority};
    VkDeviceCreateInfo info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, &all, 0, 1, &queue, 0, nullptr,
        static_cast<uint32_t>(names.size()), names.data(), nullptr};
    VkResult result = create(vulkan.physical, &info, nullptr, &vulkan.device);
    vulkan.device_proc = instance_function<PFN_vkGetDeviceProcAddr>(vulkan, "vkGetDeviceProcAddr");
    return report("vk.device", result == VK_SUCCESS ? S_OK : E_FAIL, "vk_result=" + std::to_string(result));
}
}

std::optional<VulkanDevice> create_vulkan_device() {
    VulkanDevice vulkan;
    if (!create_instance(vulkan) || !pick_device(vulkan) || !create_logical(vulkan)) return std::nullopt;
    return vulkan;
}
}
