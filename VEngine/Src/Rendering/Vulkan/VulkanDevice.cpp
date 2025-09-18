#include "VeVPCH.h"
#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan.h"
#include "VulkanDevice.h"

#define PRINT(x)        \
    {                   \
        std::cout << x; \
    }
#define PRINTLN(x)              \
    {                           \
        std::cout << x << "\n"; \
    }

namespace VEngine
{
    bool VulkanPhysicalDevice::Init(VkPhysicalDevice Device, VkSurfaceKHR Surface)
    {
        _Device = Device;
        _Indices = _FindQueueFamilies(Surface);

        VkPhysicalDeviceProperties Properties;
        vkGetPhysicalDeviceProperties(Device, &Properties);
        vkGetPhysicalDeviceFeatures(_Device, &_Features);

        _Props.Name = Properties.deviceName;
        _Props.VulkanVersion = Properties.apiVersion;

        switch (Properties.deviceType)
        {
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            _Props.DeviceType = VulkanPhysicalDeviceType::CPU;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            _Props.DeviceType = VulkanPhysicalDeviceType::DISCRETE;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            _Props.DeviceType = VulkanPhysicalDeviceType::INTEGRATED;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            _Props.DeviceType = VulkanPhysicalDeviceType::VIRTUAL;
            break;
        default:
            break;
        }

        _Props.DeviceID = Properties.deviceID;
        _Props.VendorID = Properties.vendorID;
        _Props.DriverVersion = Properties.driverVersion;
        _Limits = Properties.limits;

        return true;
    }

    void VulkanPhysicalDevice::PrintInfo()
    {
        PRINTLN("Name: " << _Props.Name);

        if (_Props.DeviceType == VulkanPhysicalDeviceType::DISCRETE)
        {
            PRINTLN("Type: Discrete GPU");
        }
        else if (_Props.DeviceType == VulkanPhysicalDeviceType::VIRTUAL)
        {
            PRINTLN("Type: Virtual GPU");
        }
        else if (_Props.DeviceType == VulkanPhysicalDeviceType::CPU)
        {
            PRINTLN("Type: CPU");
        }
        else if (_Props.DeviceType == VulkanPhysicalDeviceType::INTEGRATED)
            PRINTLN("Type: Integrated GPU");
        PRINTLN("MaxApi Versions: " << _Props.VulkanVersion)
    }

    bool VulkanPhysicalDevice::CheckExtensionsSupported(const std::vector<const char *> &Exts) const
    {
        uint32_t Count = 0;
        vkEnumerateDeviceExtensionProperties(_Device, nullptr, &Count, nullptr);

        std::vector<VkExtensionProperties> ExtProps;
        ExtProps.resize(Count);
        vkEnumerateDeviceExtensionProperties(_Device, nullptr, &Count, ExtProps.data());

        for (auto Ext : Exts)
        {
            bool Found = false;
            for (auto &Props : ExtProps)
            {
                if (strcmp(Ext, Props.extensionName) == true)
                {
                    Found = true;
                }
            }

            if (!Found)
                return false;
        }
        return true;
    }

    QueueFamilyIndices VulkanPhysicalDevice::_FindQueueFamilies(VkSurfaceKHR Surface)
    {
        uint32_t Count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(_Device, &Count, nullptr);

        std::vector<VkQueueFamilyProperties> Properties(Count);
        vkGetPhysicalDeviceQueueFamilyProperties(_Device, &Count, Properties.data());
        QueueFamilyIndices Indicies;

        int i = 0;
        for (const auto &queueFamily : Properties)
        {
            VkBool32 presentSupport = false;
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                Indicies.Queues[QueueFamilies::GRAPHICS] = i;
            vkGetPhysicalDeviceSurfaceSupportKHR(_Device, i, Surface, &presentSupport);

            if (presentSupport)
                Indicies.Queues[QueueFamilies::PRESENT] = i;
            i++;
        }
        return Indicies;
    }

    void PopulateVulkanPhysicalDevice(std::vector<VulkanPhysicalDevice> &Devices, VkInstance Instance, VkSurfaceKHR Surface)
    {
        uint32_t PhysicalDeviceCount = 0;
        vkEnumeratePhysicalDevices(Instance, &PhysicalDeviceCount, nullptr);

        Devices.reserve(PhysicalDeviceCount);
        std::vector<VkPhysicalDevice> PhysicalDevices;
        PhysicalDevices.resize(PhysicalDeviceCount);
        vkEnumeratePhysicalDevices(Instance, &PhysicalDeviceCount, PhysicalDevices.data());

        for (int i = 0; i < PhysicalDeviceCount; i++)
        {
            Devices.push_back(VulkanPhysicalDevice());
            Devices[i].Init(PhysicalDevices[i], Surface);
        }
    }

    bool VulkanDevice::Init(const VulkanDeviceSpec &Spec)
    {
        _Spec = Spec;
        QueueFamilyIndices indices = Spec.PhysicalDevice->GetQueueFamilyIndicies();

        float queuePriority = 1.0f;
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = {indices.Queues[QueueFamilies::GRAPHICS].value(), indices.Queues[QueueFamilies::PRESENT].value()};

        for (uint32_t queueFamily : uniqueQueueFamilies)
        {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.queueCreateInfoCount = queueCreateInfos.size();

        VkPhysicalDeviceFeatures deviceFeatures{};

        createInfo.pEnabledFeatures = &deviceFeatures;

        createInfo.enabledExtensionCount = 0;
        createInfo.enabledLayerCount = 0;
        if (Spec.RequiredExtensions.size() > 0)
        {
            createInfo.ppEnabledExtensionNames = Spec.RequiredExtensions.data();
            createInfo.enabledExtensionCount = Spec.RequiredExtensions.size();
        }

        if (vkCreateDevice(Spec.PhysicalDevice->GetHandle(), &createInfo, nullptr, &_Device) != VK_SUCCESS)
            throw std::runtime_error("failed to create logical device!");

        vkGetDeviceQueue(_Device, indices.Queues[QueueFamilies::GRAPHICS].value(), 0, &_Queues[QueueFamilies::GRAPHICS]);
        vkGetDeviceQueue(_Device, indices.Queues[QueueFamilies::PRESENT].value(), 0, &_Queues[QueueFamilies::PRESENT]);

        return true;
    }

    void VulkanDevice::Destroy()
    {
        vkDestroyDevice(_Device, nullptr);
        _Device = nullptr;
        _Spec.PhysicalDevice = nullptr;
        _Spec.RequiredExtensions.clear();
    }

} // namespace VEngine
