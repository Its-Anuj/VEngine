#pragma once

struct VkDebugUtilsMessengerCreateInfoEXT;

namespace VEngine
{
    struct VulkanRendererData;

    struct QueueFamilyIndices
    {
        std::optional<int> Graphics;
        std::optional<int> Present;

        inline bool Valid() { return Graphics.has_value() && Present.has_value(); }
    };

    enum class VulkanSupportedVersions
    {
        V_1_0
    };

    struct VulkanRenderSpec
    {
        std::string Name;
        VulkanSupportedVersions Version;
        std::vector<const char *> RequirerdExtensions;
        std::vector<const char *> RequirerdLayers;
        bool EnableValidationLayer = false;
        void *Win32Surface;
    };

    class VulkanRenderer
    {
    public:
        VulkanRenderer();
        ~VulkanRenderer();

        void Init(const VulkanRenderSpec &Spec);
        void Terminate();

    private:
        void _CreateInstance();
        void _CreateSuitablePhysicalDevice();
        void _CreateSuitableLogicalDevice();
        void _CreateWindowSurface();

        void _PopulatePhysicalDevices();
        void _CheckSuitablePhysicalDevice();

        QueueFamilyIndices _FindQueueFamilies(void *Device);

        void _CreateDebugMessenger();
        void _PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);
        bool _CheckSupportLayer(const std::vector<const char *> &layers);
        bool _CheckSupportExts(const std::vector<const char *> &exts);

    private:
        VulkanRendererData *_Data;
        VulkanRenderSpec _Spec;
    };
} // namespace VEngine1
