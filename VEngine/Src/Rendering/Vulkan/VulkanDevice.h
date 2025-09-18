#pragma once

namespace VEngine
{
    enum QueueFamilies
    {
        GRAPHICS,
        PRESENT,
        COUNT
    };

    struct QueueFamilyIndices
    {
        std::optional<uint32_t> Queues[QueueFamilies::COUNT];

        inline bool Valid()
        {
            for (int i = 0; i < QueueFamilies::COUNT; i++)
                if (!Queues[i].has_value())
                    return false;
            return true;
        }
    };

    enum class VulkanPhysicalDeviceType
    {
        DISCRETE,
        INTEGRATED,
        CPU,
        VIRTUAL
    };

    struct VulkanPhysicalDeviceProps
    {
        std::string Name;
        VulkanPhysicalDeviceType DeviceType;
        uint32_t DriverVersion;
        uint32_t VulkanVersion;
        uint32_t VendorID;
        uint32_t DeviceID;
    };

    class VulkanPhysicalDevice
    {
    public:
        VulkanPhysicalDevice() {}
        ~VulkanPhysicalDevice() {}

        bool Init(VkPhysicalDevice Device, VkSurfaceKHR Surface);
        void Destroy(VkInstance Instance) {}

        const VulkanPhysicalDeviceProps &GetProps() const { return _Props; }
        const VkPhysicalDeviceLimits &GetLimits() const { return _Limits; }
        const VkPhysicalDeviceFeatures &GetFeatures() const { return _Features; }
        const QueueFamilyIndices &GetQueueFamilyIndicies() const { return _Indices; }

        VkPhysicalDevice GetHandle() const { return _Device; }
        void PrintInfo();

        // const UUID &GetID() const { return _ID; }

        bool CheckExtensionsSupported(const std::vector<const char *> &Exts) const;

    private:
        QueueFamilyIndices _FindQueueFamilies(VkSurfaceKHR Surface);

        VkPhysicalDevice _Device;
        QueueFamilyIndices _Indices;
        VulkanPhysicalDeviceProps _Props;
        VkPhysicalDeviceFeatures _Features;
        VkPhysicalDeviceLimits _Limits;
        // UUID _ID;
    };

    struct VulkanDeviceSpec
    {
        std::vector<const char *> RequiredExtensions;
        VulkanPhysicalDevice *PhysicalDevice = nullptr;
    };

    class VulkanDevice
    {
    public:
        VulkanDevice() {}
        ~VulkanDevice() {}

        bool Init(const VulkanDeviceSpec &Spec);
        void Destroy();
        // const UUID &GetID() const { return _ID; }
        VkDevice GetHandle() const { return _Device; }

        VkQueue GetQueues(QueueFamilies i) const { return _Queues[i]; }
        const VulkanPhysicalDevice *GetPhysicalDevice() const { return _Spec.PhysicalDevice; }

    private:
        // UUID _ID;
        // TODO: Make it use UUIDS
        VulkanDeviceSpec _Spec;
        VkDevice _Device = nullptr;
        VkQueue _Queues[QueueFamilies::COUNT];
    };

    void PopulateVulkanPhysicalDevice(std::vector<VulkanPhysicalDevice> &Devices, VkInstance Instance, VkSurfaceKHR Surface);
} // namespace VEngine
