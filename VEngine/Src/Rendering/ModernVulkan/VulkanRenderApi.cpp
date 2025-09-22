#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan/vulkan.h"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#include "VulkanRenderApi.h"

#define PRINTLN(x)      \
    {                   \
        std::cout << x; \
    }
#define PRINTLN(x)              \
    {                           \
        std::cout << x << "\n"; \
    }
#define VULKAN_SUCCESS_ASSERT(x, errmsg)      \
    {                                         \
        if (x != VK_SUCCESS)                  \
            throw std::runtime_error(errmsg); \
    }

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pDebugMessenger);
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks *pAllocator);

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *pUserData)
{

    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

namespace VEngine
{
    namespace VulkanUtils
    {
        void GetInstanceExtensions(std::vector<VkExtensionProperties> &Props)
        {
            uint32_t extcount;
            vkEnumerateInstanceExtensionProperties(nullptr, &extcount, nullptr);

            Props.resize(extcount);
            vkEnumerateInstanceExtensionProperties(nullptr, &extcount, Props.data());
        }

        void GetInstanceLayers(std::vector<VkLayerProperties> &Props)
        {
            uint32_t count;
            vkEnumerateInstanceLayerProperties(&count, nullptr);

            Props.resize(count);
            vkEnumerateInstanceLayerProperties(&count, Props.data());
        }

        bool CheckIfExtensionExist(std::vector<const char *> &Extensions)
        {
            std::vector<VkExtensionProperties> ExtProps;
            GetInstanceExtensions(ExtProps);

            for (auto Ext : Extensions)
            {
                bool found = false;
                for (auto &props : ExtProps)
                {
                    if (strcmp(props.extensionName, Ext))
                    {
                        found = true;
                        break;
                    }
                }

                if (!found)
                    return false;
            }
            return true;
        }
        bool CheckIfLayerExist(std::vector<const char *> &Layers)
        {
            std::vector<VkLayerProperties> LayerProps;
            GetInstanceLayers(LayerProps);

            for (auto layer : Layers)
            {
                bool found = false;
                for (auto &props : LayerProps)
                {
                    if (strcmp(props.layerName, layer))
                    {
                        found = true;
                        break;
                    }
                }

                if (!found)
                {
                    PRINTLN("[VULKAN]: " << layer << " not Found!");
                    return false;
                }
            }
            return true;
        }
    } // namespace VulkanUtils

    enum QueueFamilies
    {
        GRAPHICS,
        PRESENT,
        TRANSFER,
        COMPUTE,
        COUNT
    };

    struct QueueFamilyIndicies
    {
        std::optional<uint32_t> Queues[int(QueueFamilies::COUNT)];

        inline bool Complete() const { return Queues[int(QueueFamilies::GRAPHICS)].has_value() &
                                              Queues[int(QueueFamilies::PRESENT)].has_value() & Queues[int(QueueFamilies::TRANSFER)].has_value(); }
    };

    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
    {
        SwapChainSupportDetails details;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

        if (formatCount != 0)
        {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        }

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

        if (presentModeCount != 0)
        {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
        }

        return details;
    }

    struct VulkanPhysiscalDeviceInfo
    {
        VkPhysicalDeviceProperties properties;
        VkPhysicalDeviceFeatures features;
        VkPhysicalDeviceVulkan11Features features11; // Vulkan 1.1
        VkPhysicalDeviceVulkan12Features features12; // Vulkan 1.2
        VkPhysicalDeviceVulkan13Features features13; // Vulkan 1.3
        VkPhysicalDeviceMemoryProperties memoryProperties;
        QueueFamilyIndicies QueueIndicies;
        SwapChainSupportDetails SwapChainDetails;
        bool SupportsExtensions = true;
    };

    struct VulkanRenderData
    {
        // Inital Setups
        VkInstance Instance;
        VkDebugUtilsMessengerEXT DebugMessenger;
        VkSurfaceKHR SurfaceKHR;

        // Devices
        std::vector<VkPhysicalDevice> PDevices;
        std::vector<VulkanPhysiscalDeviceInfo> PDevicesInfo;
        int ActivePhysicalDeviceIndex = 0;

        VkDevice ActiveDevice;
        VkQueue Queues[int(QueueFamilies::COUNT)];
    };
#pragma region VulkanRenderApi
    void VulkanRenderApi::Init(void *Spec)
    {
        _Data = new VulkanRenderData();
        _Spec = *(VulkanRenderSpec *)Spec;

        _CreateInstance();
        _CreateDebugMessenger();
        _CreateWin32Surface();

        _CreatePhysicalDevice();
        _CreateLogicalDevice();

        PRINTLN("[VULKAN]: New Modern Render Api Created!!");
        _ResourceFactory = std::make_shared<VulkanResourceFactory>(_Data);
        PRINTLN("[VULKAN]: Vulkan Resource Factory Api Created!!");
    }

    void VulkanRenderApi::Terminate()
    {
        _ResourceFactory->Terminate();
        PRINTLN("[VULKAN]: Vulkan Resource Factory Api Terminated!!");
        
        vkDestroyDevice(_Data->ActiveDevice, nullptr);

        vkDestroySurfaceKHR(_Data->Instance, _Data->SurfaceKHR, nullptr);

        if (_Spec.EnableValidationLayer)
            DestroyDebugUtilsMessengerEXT(_Data->Instance, _Data->DebugMessenger, nullptr);

        vkDestroyInstance(_Data->Instance, nullptr);
        PRINTLN("[VULKAN]: Modern Render Api Terminated !!");
    }

    void VulkanRenderApi::_CreateInstance()
    {
        std::vector<const char *> RequiredExts, RequiredLayers;
        RequiredExts.push_back("VK_KHR_win32_surface");
        RequiredExts.push_back("VK_KHR_surface");

        if (_Spec.EnableValidationLayer)
        {
            // Check if validation layer exists
            RequiredLayers.push_back("VK_LAYER_KHRONOS_validation");
            RequiredExts.push_back("VK_EXT_debug_utils");
        }

        if (!VulkanUtils::CheckIfLayerExist(RequiredLayers))
        {
            PRINTLN("[VULKAN] : All layers not supported!")
            return;
        }
        if (!VulkanUtils::CheckIfExtensionExist(RequiredExts))
        {
            PRINTLN("[VULKAN] : All layers not supported!")
            return;
        }

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = _Spec.Name.c_str();

        appInfo.applicationVersion = VK_MAKE_VERSION(1, 3, 0);
        appInfo.engineVersion = VK_MAKE_VERSION(1, 3, 0);
        appInfo.apiVersion = VK_API_VERSION_1_3;

        appInfo.pEngineName = "No Engine";

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        createInfo.enabledLayerCount = 0;
        createInfo.enabledExtensionCount = RequiredExts.size();
        createInfo.ppEnabledExtensionNames = RequiredExts.data();
        createInfo.enabledLayerCount = RequiredLayers.size();
        createInfo.pNext = nullptr;

        if (RequiredLayers.size() == 0)
            createInfo.ppEnabledLayerNames = nullptr;
        else
            createInfo.ppEnabledLayerNames = RequiredLayers.data();

        if (_Spec.EnableValidationLayer)
        {
            VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

            _PopulateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
        }

        VULKAN_SUCCESS_ASSERT(vkCreateInstance(&createInfo, nullptr, &_Data->Instance), "Vulkan Instance Failed!")
        PRINTLN("Init vulkan!")
    }

    void VulkanRenderApi::_CreateDebugMessenger()
    {
        if (!_Spec.EnableValidationLayer)
            return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        _PopulateDebugMessengerCreateInfo(createInfo);

        VULKAN_SUCCESS_ASSERT(CreateDebugUtilsMessengerEXT(_Data->Instance, &createInfo, nullptr, &_Data->DebugMessenger), "Debug Messenger not loaded!");
        PRINTLN("[VULKAN]: Debug Messenger Loaded!")
    }

    void VulkanRenderApi::_PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo)
    {
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
        createInfo.pUserData = nullptr; // Optional
        createInfo.pNext = nullptr;     // Optional
        createInfo.flags = 0;           // Must be 0
    }

    void VulkanRenderApi::_CreateWin32Surface()
    {
        VkWin32SurfaceCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        createInfo.hwnd = (HWND)_Spec.Win32Surface;
        createInfo.hinstance = GetModuleHandle(nullptr);

        if (vkCreateWin32SurfaceKHR(_Data->Instance, &createInfo, nullptr, &_Data->SurfaceKHR) != VK_SUCCESS)
            throw std::runtime_error("failed to create window surface!");
        PRINTLN("[VULKAN]: Window Surface KHR Created!")
    }

    void VulkanRenderApi::_CreatePhysicalDevice()
    {
        uint32_t PDeviceCount = 0;
        vkEnumeratePhysicalDevices(_Data->Instance, &PDeviceCount, nullptr);

        _Data->PDevices.resize(PDeviceCount);
        vkEnumeratePhysicalDevices(_Data->Instance, &PDeviceCount, _Data->PDevices.data());

        if (_Data->PDevices.size() == 0)
            throw std::runtime_error("No Vulkan Supporting device found!");

        PRINTLN("[VULKAN]: Vulkan Supported devices: " << _Data->PDevices.size())
        // Check how good each phsyical device is in features
        _FillPhysicalDeviceInfo();

        _Spec.DeviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                                  "VK_KHR_dynamic_rendering",
                                  "VK_KHR_synchronization2",
                                  "VK_KHR_maintenance4",
                                  "VK_EXT_pipeline_creation_cache_control",
                                  "VK_EXT_shader_demote_to_helper_invocation",
                                  "VK_KHR_shader_terminate_invocation",
                                  "VK_EXT_sampler_filter_minmax",
                                  "VK_KHR_buffer_device_address"};

        _CheckPhysicalDeviceExtensions(_Spec.DeviceExtensions);
        _FindSuitablePhysicalDevice();
    }

    void VulkanRenderApi::_FillPhysicalDeviceInfo()
    {
        _Data->PDevicesInfo.reserve(_Data->PDevices.size());

        for (auto device : _Data->PDevices)
        {
            VulkanPhysiscalDeviceInfo info{};

            vkGetPhysicalDeviceFeatures(device, &info.features);
            vkGetPhysicalDeviceProperties(device, &info.properties);
            vkGetPhysicalDeviceMemoryProperties(device, &info.memoryProperties);

            info.features11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
            info.features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
            info.features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;

            VkPhysicalDeviceFeatures2 features2{};
            features2.pNext = &info.features11;
            info.features11.pNext = &info.features12;
            info.features12.pNext = &info.features13;
            features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

            vkGetPhysicalDeviceFeatures2(device, &features2);

            // Queue families
            uint32_t queufamiliescount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queufamiliescount, nullptr);

            std::vector<VkQueueFamilyProperties> QueueProps;
            QueueProps.resize(queufamiliescount);
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queufamiliescount, QueueProps.data());

            int i = 0;
            for (auto &props : QueueProps)
            {
                if (props.queueFlags & VK_QUEUE_COMPUTE_BIT)
                    info.QueueIndicies.Queues[int(QueueFamilies::COMPUTE)] = i;
                if (props.queueFlags & VK_QUEUE_TRANSFER_BIT)
                    info.QueueIndicies.Queues[int(QueueFamilies::TRANSFER)] = i;
                if (props.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                    info.QueueIndicies.Queues[int(QueueFamilies::GRAPHICS)] = i;

                VkBool32 presentSupport;
                vkGetPhysicalDeviceSurfaceSupportKHR(device, i, _Data->SurfaceKHR, &presentSupport);

                if (presentSupport)
                    info.QueueIndicies.Queues[QueueFamilies::PRESENT] = i;
                i++;
            }

            info.SwapChainDetails = QuerySwapChainSupport(device, _Data->SurfaceKHR);

            _Data->PDevicesInfo.push_back(info);
            PRINTLN("[VULKAN]: Device Found: " << info.properties.deviceName << "\n");
        }
    }

    void VulkanRenderApi::_FindSuitablePhysicalDevice()
    {
        int Scores[_Data->PDevices.size()];

        int i = 0;
        for (auto &deviceinfo : _Data->PDevicesInfo)
        {
            int CurrentScore = 0;

            if (deviceinfo.features13.dynamicRendering == true)
                CurrentScore += 1000;
            if (deviceinfo.features13.synchronization2 == true)
                CurrentScore += 1000;
            if (deviceinfo.features13.maintenance4 == true)
                CurrentScore += 1000;
            if (deviceinfo.features13.pipelineCreationCacheControl)
                CurrentScore += 1000;
            if (deviceinfo.features13.shaderDemoteToHelperInvocation)
                CurrentScore += 1000;
            if (deviceinfo.features13.shaderTerminateInvocation)
                CurrentScore += 1000;
            if (deviceinfo.features12.samplerFilterMinmax)
                CurrentScore += 1000;
            if (deviceinfo.features12.bufferDeviceAddressCaptureReplay)
                CurrentScore += 1000;

            if (deviceinfo.features.geometryShader)
                CurrentScore += 1000;
            if (deviceinfo.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
                CurrentScore += 10000;
            if (deviceinfo.QueueIndicies.Complete())
                CurrentScore += 10000;

            if ((!deviceinfo.SwapChainDetails.formats.empty() && !deviceinfo.SwapChainDetails.presentModes.empty()) == true)
                CurrentScore += 10000;
            PRINTLN("[VULKAN]: " << deviceinfo.properties.deviceName << " scored: " << CurrentScore);
            Scores[i++] = CurrentScore;
        }

        int Bestid = 0;
        for (int i = 0; i < _Data->PDevices.size(); i++)
        {
            if (Scores[Bestid] < Scores[i])
                Bestid = i;
        }
        PRINTLN("Chosen Device: " << _Data->PDevicesInfo[Bestid].properties.deviceName << "\n")
        _Data->ActivePhysicalDeviceIndex = Bestid;
    }

    void VulkanRenderApi::_CheckPhysicalDeviceExtensions(std::vector<const char *> &PDeviceExtProps)
    {
        int i = 0;
        for (auto &device : _Data->PDevices)
        {
            auto &deviceprops = _Data->PDevicesInfo[i];

            uint32_t count = 0;
            vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);

            std::vector<VkExtensionProperties> ExtProps;
            ExtProps.resize(count);
            vkEnumerateDeviceExtensionProperties(device, nullptr, &count, ExtProps.data());

            for (auto name : PDeviceExtProps)
            {
                bool found = false;
                for (auto &props : ExtProps)
                {
                    if (strcmp(props.extensionName, name))
                        found = true;
                }

                if (!found)
                {
                    deviceprops.SupportsExtensions = false;
                }
            }
        }
    }

    void VulkanRenderApi::_CreateLogicalDevice()
    {
        auto indices = _Data->PDevicesInfo[_Data->ActivePhysicalDeviceIndex].QueueIndicies;

        float queuePriority = 1.0f;
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = {indices.Queues[QueueFamilies::GRAPHICS].value(), indices.Queues[QueueFamilies::PRESENT].value(),
                                                  indices.Queues[QueueFamilies::COMPUTE].value(), indices.Queues[QueueFamilies::TRANSFER].value()};

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

        createInfo.enabledExtensionCount = _Spec.DeviceExtensions.size();
        createInfo.ppEnabledExtensionNames = _Spec.DeviceExtensions.data();

        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.queueCreateInfoCount = queueCreateInfos.size();

        // ALl modern req features
        VkPhysicalDeviceVulkan13Features features13{};
        features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
        features13.dynamicRendering = VK_TRUE;
        features13.synchronization2 = VK_TRUE;
        features13.maintenance4 = VK_TRUE;
        features13.pipelineCreationCacheControl = VK_TRUE;
        features13.shaderDemoteToHelperInvocation = VK_TRUE;
        features13.shaderTerminateInvocation = VK_TRUE;
        features13.pNext = nullptr; // next in chain

        VkPhysicalDeviceVulkan12Features features12{};
        features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        features12.samplerFilterMinmax = VK_TRUE;
        features12.bufferDeviceAddressCaptureReplay = VK_TRUE;
        features12.pNext = &features13; // chain 1.3 features after 1.2

        VkPhysicalDeviceFeatures features{};
        createInfo.pEnabledFeatures = &features;

        createInfo.enabledLayerCount = 0;
        createInfo.pNext = &features12;

        if (vkCreateDevice(_Data->PDevices[_Data->ActivePhysicalDeviceIndex], &createInfo, nullptr, &_Data->ActiveDevice) != VK_SUCCESS)
            throw std::runtime_error("failed to create logical device!");

        vkGetDeviceQueue(_Data->ActiveDevice, indices.Queues[QueueFamilies::GRAPHICS].value(), 0, &_Data->Queues[QueueFamilies::GRAPHICS]);
        vkGetDeviceQueue(_Data->ActiveDevice, indices.Queues[QueueFamilies::PRESENT].value(), 0, &_Data->Queues[QueueFamilies::PRESENT]);

        if (indices.Queues[QueueFamilies::TRANSFER].has_value())
            vkGetDeviceQueue(_Data->ActiveDevice, indices.Queues[QueueFamilies::TRANSFER].value(), 0, &_Data->Queues[QueueFamilies::TRANSFER]);
        if (indices.Queues[QueueFamilies::COMPUTE].has_value())
            vkGetDeviceQueue(_Data->ActiveDevice, indices.Queues[QueueFamilies::COMPUTE].value(), 0, &_Data->Queues[QueueFamilies::COMPUTE]);
    }

#pragma endregion

#pragma region VulkanResourceFactory
    VulkanResourceFactory::~VulkanResourceFactory()
    {
    }

    void VulkanResourceFactory::Terminate()
    {
        _Data = nullptr;
    }

    Ref<VertexBuffer> VulkanResourceFactory::CreateVertexBuffer(const VertexBufferDesc &desc)
    {
        return Ref<VertexBuffer>();
    }

    Ref<IndexBuffer> VulkanResourceFactory::CreateIndexBuffer(const IndexBufferDesc &desc)
    {
        return Ref<IndexBuffer>();
    }

    Ref<Shader> VulkanResourceFactory::CreateGraphicsPipeline(const GraphicsShaderDesc &desc)
    {
        return Ref<Shader>();
    }
#pragma endregion
} // namespace VEngine

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
