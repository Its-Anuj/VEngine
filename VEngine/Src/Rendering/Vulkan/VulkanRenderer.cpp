#include "VeVPCH.h"
#include "VulkanRenderer.h"
#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan.h"

#define PRINT(x)        \
    {                   \
        std::cout << x; \
    }
#define PRINTLN(x)              \
    {                           \
        std::cout << x << "\n"; \
    }

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *pUserData)
{

    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pDebugMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks *pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        func(instance, debugMessenger, pAllocator);
    }
}

namespace VEngine
{

    struct VulkanRendererData
    {
        VkInstance Instance;
        VkDebugUtilsMessengerEXT DebugMessenger;
        int ActivePhysicalDeviceIndex;
        int ActiveDeviceIndex;
        QueueFamilyIndices QueueIndicies;
        VkSurfaceKHR Surface;

        // Abstract to classes (RAII)
        std::vector<VkPhysicalDevice> PhysicalDevices;
        std::vector<VkDevice> Devices;
        VkQueue graphicsQueue;
    };

    VulkanRenderer::VulkanRenderer()
    {
    }

    VulkanRenderer::~VulkanRenderer()
    {
        delete _Data;
    }

    void VulkanRenderer::Terminate()
    {
        for (auto Device : _Data->Devices)
            vkDestroyDevice(Device, nullptr);

        if (_Spec.EnableValidationLayer)
            DestroyDebugUtilsMessengerEXT(_Data->Instance, _Data->DebugMessenger, nullptr);

        vkDestroySurfaceKHR(_Data->Instance, _Data->Surface, nullptr);

        vkDestroyInstance(_Data->Instance, nullptr);
        PRINTLN("Terminated vulkan!")
    }

    void VulkanRenderer::Init(const VulkanRenderSpec &Spec)
    {
        _Data = new VulkanRendererData();
        _Spec = Spec;

        _CreateInstance();
        _CreateDebugMessenger();
        _CreateSuitablePhysicalDevice();
        _CreateSuitableLogicalDevice();
    }

    void VulkanRenderer::_CreateInstance()
    {
        if (_Spec.EnableValidationLayer)
        {
            _Spec.RequirerdLayers.push_back("VK_LAYER_KHRONOS_validation");
            _Spec.RequirerdExtensions.push_back("VK_EXT_debug_utils");
        }

        if (_Spec.RequirerdLayers.size() > 0)
            if (_CheckSupportLayer(_Spec.RequirerdLayers) == false)
            {
                PRINTLN("Doesnot Supoport all Layers!")
                return;
            }

        // Check for extensions

        if (_Spec.RequirerdExtensions.size() > 0)
            if (_CheckSupportExts(_Spec.RequirerdExtensions) == false)
            {
                PRINTLN("Doesnot Supoport all Extensions!")
                return;
            }

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = _Spec.Name.c_str();

        if (_Spec.Version == VulkanSupportedVersions::V_1_0)
        {
            appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
            appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
            appInfo.apiVersion = VK_API_VERSION_1_0;
        }

        appInfo.pEngineName = "No Engine";

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        createInfo.enabledLayerCount = 0;
        createInfo.enabledExtensionCount = _Spec.RequirerdExtensions.size();
        createInfo.ppEnabledExtensionNames = _Spec.RequirerdExtensions.data();
        createInfo.enabledLayerCount = _Spec.RequirerdLayers.size();
        createInfo.pNext = nullptr;

        if (_Spec.RequirerdLayers.size() == 0)
            createInfo.ppEnabledLayerNames = nullptr;
        else
            createInfo.ppEnabledLayerNames = _Spec.RequirerdLayers.data();

        if (_Spec.EnableValidationLayer)
        {
            VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

            _PopulateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
        }

        if (vkCreateInstance(&createInfo, nullptr, &_Data->Instance) != VK_SUCCESS)
        {
            throw std::runtime_error("Vulkan couldnot be initialized!");
        }
        PRINTLN("Init vulkan!")
    }

    void VulkanRenderer::_CreateSuitablePhysicalDevice()
    {
        _PopulatePhysicalDevices();
        _CheckSuitablePhysicalDevice();
    }

    void VulkanRenderer::_CreateSuitableLogicalDevice()
    {
        int i = 0;
        for (auto Device : _Data->PhysicalDevices)
        {
            QueueFamilyIndices indices = _FindQueueFamilies(Device);

            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = indices.Graphics.value();
            queueCreateInfo.queueCount = 1;
            float queuePriority = 1.0f;
            queueCreateInfo.pQueuePriorities = &queuePriority;

            VkDeviceCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            createInfo.pQueueCreateInfos = &queueCreateInfo;
            createInfo.queueCreateInfoCount = 1;

            VkPhysicalDeviceFeatures deviceFeatures{};

            createInfo.pEnabledFeatures = &deviceFeatures;

            createInfo.enabledExtensionCount = 0;

            if (_Spec.RequirerdLayers.size() > 0)
            {
                createInfo.ppEnabledLayerNames = _Spec.RequirerdLayers.data();
                createInfo.enabledLayerCount = _Spec.RequirerdLayers.size();
            }

            _Data->Devices.push_back(nullptr);
            if (vkCreateDevice(Device, &createInfo, nullptr, &_Data->Devices[i]) != VK_SUCCESS)
                throw std::runtime_error("failed to create logical device!");
            PRINTLN("[VULKAN]: Logical Device Created!")

            i++;
        }
        _Data->ActiveDeviceIndex = _Data->ActivePhysicalDeviceIndex;

        QueueFamilyIndices indices = _FindQueueFamilies(_Data->PhysicalDevices[_Data->ActivePhysicalDeviceIndex]);
        vkGetDeviceQueue(_Data->Devices[_Data->ActiveDeviceIndex], indices.Graphics.value(), 0, &_Data->graphicsQueue);
    }

    void VulkanRenderer::_CreateWindowSurface()
    {
        VkWin32SurfaceCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        createInfo.hwnd = (HWND)_Spec.Win32Surface;
        createInfo.hinstance = GetModuleHandle(nullptr);

        if (vkCreateWin32SurfaceKHR(_Data->Instance, &createInfo, nullptr, &_Data->Surface) != VK_SUCCESS)
            throw std::runtime_error("failed to create window surface!");
        PRINTLN("[VULKAN]: Window Surface KHR Created!")
    }

    void VulkanRenderer::_PopulatePhysicalDevices()
    {
        uint32_t PhysicalDeviceCount = 0;
        vkEnumeratePhysicalDevices(_Data->Instance, &PhysicalDeviceCount, nullptr);

        _Data->PhysicalDevices.resize(PhysicalDeviceCount);
        vkEnumeratePhysicalDevices(_Data->Instance, &PhysicalDeviceCount, _Data->PhysicalDevices.data());
    }

    void VulkanRenderer::_CheckSuitablePhysicalDevice()
    {
        int Scores[_Data->PhysicalDevices.size()] = {0};

        int i = 0;

        for (auto Device : _Data->PhysicalDevices)
        {
            auto Indicies = _FindQueueFamilies(Device);

            VkPhysicalDeviceProperties Properties;
            VkPhysicalDeviceFeatures Features;
            vkGetPhysicalDeviceProperties(Device, &Properties);
            vkGetPhysicalDeviceFeatures(Device, &Features);

            PRINTLN("Name: " << Properties.deviceName);

            if (Properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            {
                PRINTLN("Type: Discrete GPU");
            }
            else if (Properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU)
            {
                PRINTLN("Type: Virtual GPU");
            }
            else if (Properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU)
            {
                PRINTLN("Type: CPU");
            }
            else if (Properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
                PRINTLN("Type: Integrated GPU");

            if (Properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
                Scores[i] += 1000;

            Scores[i] += Properties.limits.maxImageDimension2D;

            if (!Features.geometryShader)
                Scores[i] = 0;
            if (Indicies.Graphics.has_value())
                Scores[i] += 10000;
            i++;
        }

        int Bestid = 0;
        for (int i = 0; i < _Data->PhysicalDevices.size(); i++)
        {
            if (Scores[Bestid] < Scores[i])
                Bestid = i;
        }
        _Data->ActivePhysicalDeviceIndex = Bestid;
    }

    QueueFamilyIndices VulkanRenderer::_FindQueueFamilies(void *Device)
    {
        VkPhysicalDevice device = (VkPhysicalDevice)Device;
        uint32_t Count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &Count, nullptr);

        std::vector<VkQueueFamilyProperties> Properties(Count);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &Count, Properties.data());
        QueueFamilyIndices Indicies;

        int i = 0;
        for (const auto &queueFamily : Properties)
        {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                Indicies.Graphics = i;

            i++;
        }
        return Indicies;
    }

    void VulkanRenderer::_CreateDebugMessenger()
    {
        if (!_Spec.EnableValidationLayer)
            return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        _PopulateDebugMessengerCreateInfo(createInfo);

        if (CreateDebugUtilsMessengerEXT(_Data->Instance, &createInfo, nullptr, &_Data->DebugMessenger) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to set up debug messenger!");
        }
    }

    void VulkanRenderer::_PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo)
    {
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
        createInfo.pUserData = nullptr; // Optional
        createInfo.pNext = nullptr;     // Optional
        createInfo.flags = 0;           // Must be 0
    }

    bool VulkanRenderer::_CheckSupportLayer(const std::vector<const char *> &layers)
    {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char *Checklayer : layers)
        {
            bool Found = false;
            for (auto &Layer : availableLayers)
            {
                if (strcmp(Checklayer, Layer.layerName) == true)
                {
                    Found = true;
                }
            }

            if (Found == false)
            {
                PRINTLN("[VULKAN]: Layer: " << Checklayer << " not found!")
                return false;
            }

            PRINTLN("[VULKAN]: Layer: " << Checklayer << " found!")
        }

        return true;
    }

    bool VulkanRenderer::_CheckSupportExts(const std::vector<const char *> &exts)
    {
        uint32_t ExtensionsCount;
        vkEnumerateInstanceExtensionProperties(NULL, &ExtensionsCount, nullptr);

        std::vector<VkExtensionProperties> AvailableExt(ExtensionsCount);
        vkEnumerateInstanceExtensionProperties(NULL, &ExtensionsCount, AvailableExt.data());

        for (const char *ExtName : exts)
        {
            bool ExtFound = false;

            for (const auto &ExtensionsProperties : AvailableExt)
            {
                if (strcmp(ExtName, ExtensionsProperties.extensionName) == 0)
                {
                    ExtFound = true;
                    PRINTLN("[VULKAN]: Supports: " << ExtName << " Extension")
                    break;
                }
            }

            if (!ExtFound)
            {
                PRINTLN("[VULKAN]: Not Supported: " << ExtName << " Extension")
                return false;
            }
        }

        return true;
    }
} // namespace VEngine
