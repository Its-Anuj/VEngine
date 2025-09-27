#pragma once

namespace VEngine
{
    namespace VulkanUtils
    {
        void GetInstanceExtensions(std::vector<VkExtensionProperties> &Props);

        void GetInstanceLayers(std::vector<VkLayerProperties> &Props);

        bool CheckIfExtensionExist(std::vector<const char *> &Extensions);
        bool CheckIfLayerExist(std::vector<const char *> &Layers);
        VulkanVersion GetInstanceVulkanVersion();

        VkFormatProperties GetPhysicalDeviceFormatProperties(VulkanPhysicalDevice& Device, VkFormat Format);
    } // namespace VulkanUtils
} // namespace VEngine
