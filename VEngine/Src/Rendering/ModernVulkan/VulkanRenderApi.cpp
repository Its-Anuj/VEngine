#pragma once

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

    struct VulkanRenderData
    {
        VkInstance Instance;
    };

    void VulkanRenderApi::Init(void *Spec)
    {
        _Data = new VulkanRenderData();
        _Spec = *(VulkanRenderSpec *)Spec;

        _CreateInstance();
        _ResourceFactory = std::make_shared<VulkanResourceFactory>(_Data);
        PRINTLN("[VULKAN]: New Modern Render Api Created!!");
    }

    void VulkanRenderApi::Terminate()
    {
        _ResourceFactory->Terminate();
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

        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

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
            // VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

            // _PopulateDebugMessengerCreateInfo(debugCreateInfo);
            // createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
        }

        VULKAN_SUCCESS_ASSERT(vkCreateInstance(&createInfo, nullptr, &_Data->Instance), "Vulkan Instance Failed!")
        PRINTLN("Init vulkan!")
    }

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
} // namespace VEngine
