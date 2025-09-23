#include "VeVPCH.h"

#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan/vulkan.h"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#include "VulkanRenderApi.h"
#include "VulkanDevice.h"

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

    struct VulkanRenderData
    {
        // Inital Setups
        VkInstance Instance;
        VkDebugUtilsMessengerEXT DebugMessenger;
        VkSurfaceKHR SurfaceKHR;

        // Devices
        std::vector<VulkanPhysicalDevice> PhysicalDevices;
        int ActivePhysicalDeviceIndex = 0;

        VulkanDevice Device;

        VmaAllocator Allocator;

        VkSwapchainKHR SwapChain;
        VkFormat Format;
        VkExtent2D Extent;

        std::vector<VkImage> SwapChainImages;
        std::vector<VkImageView> SwapChainImageViews;

        struct
        {
            int x, y;
        } FrameBufferSize;

        VkPipeline GraphcisPipeline;
        VkPipelineLayout GraphcisPipelineLayout;

        VkCommandPool GraphicsCommandPool;
        std::vector<VkCommandBuffer> GraphicsCommandBuffers;
        std::vector<VkSemaphore> ImageAvailableSemaphores;
        std::vector<VkSemaphore> RenderFinishedSemaphores;
        std::vector<VkFence> InFlightFences;
        uint32_t CurrentFrame = 0;
        uint32_t CurrentImageIndex = 0;
    };
#pragma region VulkanRenderApi
    void VulkanRenderApi::Init(void *Spec)
    {
        _Data = new VulkanRenderData();
        _Spec = *(VulkanRenderSpec *)Spec;

        _Spec.DeviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                                  "VK_KHR_dynamic_rendering",
                                  "VK_KHR_synchronization2",
                                  "VK_KHR_maintenance4",
                                  "VK_EXT_pipeline_creation_cache_control",
                                  "VK_EXT_shader_demote_to_helper_invocation",
                                  "VK_KHR_shader_terminate_invocation",
                                  "VK_EXT_sampler_filter_minmax",
                                  "VK_KHR_buffer_device_address"};
        _Data->FrameBufferSize.x = _Spec.FrameBufferSize.x;
        _Data->FrameBufferSize.y = _Spec.FrameBufferSize.y;

        _CreateInstance();
        _CreateDebugMessenger();
        _CreateWin32Surface();

        _CreatePhysicalDevice();
        _CreateLogicalDevice();

        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.physicalDevice = _Data->PhysicalDevices[_Data->ActivePhysicalDeviceIndex].PhysicalDevice;
        allocatorInfo.device = _Data->Device.GetHandle();
        allocatorInfo.instance = _Data->Instance;

        VULKAN_SUCCESS_ASSERT(vmaCreateAllocator(&allocatorInfo, &_Data->Allocator), "VMA Allocator failed to initialize!");
        PRINTLN("[VULKAN]: VMA Created!!");

        _CreateSwapChain();

        _CreateGraphiscPipeline();

        _CreateCommandPool();
        _CreateCommandBuffer();
        _CreateSyncObjects();

        PRINTLN("[VULKAN]: New Modern Render Api Created!!");
        _ResourceFactory = std::make_shared<VulkanResourceFactory>(_Data);
        PRINTLN("[VULKAN]: Vulkan Resource Factory Api Created!!");
    }

    void VulkanRenderApi::Terminate()
    {
        vkDeviceWaitIdle(_Data->Device.GetHandle());
        _ResourceFactory->Terminate();
        PRINTLN("[VULKAN]: Vulkan Resource Factory Api Terminated!!");

        vmaDestroyAllocator(_Data->Allocator);
        for (size_t i = 0; i < _Spec.InFrameFlightCount; i++)
        {
            vkDestroySemaphore(_Data->Device.GetHandle(), _Data->ImageAvailableSemaphores[i], nullptr);
            vkDestroySemaphore(_Data->Device.GetHandle(), _Data->RenderFinishedSemaphores[i], nullptr);
            vkDestroyFence(_Data->Device.GetHandle(), _Data->InFlightFences[i], nullptr);
        }
        _Data->ImageAvailableSemaphores.clear();
        _Data->RenderFinishedSemaphores.clear();
        _Data->InFlightFences.clear();
        // Command buffers are automatically freed when command pool is destroyed
        // But we can explicitly free them if we want to recreate them
        if (!_Data->GraphicsCommandBuffers.empty() && _Data->GraphicsCommandPool != VK_NULL_HANDLE)
        {
            vkFreeCommandBuffers(_Data->Device.GetHandle(), _Data->GraphicsCommandPool,
                                 static_cast<uint32_t>(_Data->GraphicsCommandBuffers.size()), _Data->GraphicsCommandBuffers.data());
            _Data->GraphicsCommandBuffers.clear();
        }

        vkDestroyCommandPool(_Data->Device.GetHandle(), _Data->GraphicsCommandPool, nullptr);

        vkDestroyPipeline(_Data->Device.GetHandle(), _Data->GraphcisPipeline, nullptr);
        vkDestroyPipelineLayout(_Data->Device.GetHandle(), _Data->GraphcisPipelineLayout, nullptr);

        for (auto imageview : _Data->SwapChainImageViews)
            vkDestroyImageView(_Data->Device.GetHandle(), imageview, nullptr);
        vkDestroySwapchainKHR(_Data->Device.GetHandle(), _Data->SwapChain, nullptr);

        _Data->Device.Destroy();
        vkDestroySurfaceKHR(_Data->Instance, _Data->SurfaceKHR, nullptr);

        if (_Spec.EnableValidationLayer)
            DestroyDebugUtilsMessengerEXT(_Data->Instance, _Data->DebugMessenger, nullptr);

        vkDestroyInstance(_Data->Instance, nullptr);
        PRINTLN("[VULKAN]: Modern Render Api Terminated !!");
    }

    void VulkanRenderApi::Render()
    {
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = {_Data->ImageAvailableSemaphores[_Data->CurrentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &_Data->GraphicsCommandBuffers[_Data->CurrentFrame];

        VkSemaphore signalSemaphores[] = {_Data->RenderFinishedSemaphores[_Data->CurrentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(_Data->Device.GetQueue(QueueFamilies::GRAPHICS), 1, &submitInfo, _Data->InFlightFences[_Data->CurrentFrame]) != VK_SUCCESS)
            throw std::runtime_error("failed to submit draw command buffer!");
    }

    void VulkanRenderApi::Submit()
    {
        auto commandBuffer = _Data->GraphicsCommandBuffers[_Data->CurrentFrame];
        // Bind pipeline (created without render pass)
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _Data->GraphcisPipeline);

        // Set dynamic viewport (matches pipeline dynamic state)
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(_Data->Extent.width);
        viewport.height = static_cast<float>(_Data->Extent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        // Set dynamic scissor
        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = _Data->Extent;
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        // 🎨 Draw your geometry
        vkCmdDraw(commandBuffer, 3, 1, 0, 0); // Draw a triangle
    }

    void VulkanRenderApi::Present()
    {
        VkSemaphore signalSemaphores[] = {_Data->RenderFinishedSemaphores[_Data->CurrentFrame]};

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = {_Data->SwapChain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &_Data->CurrentImageIndex;
        presentInfo.pResults = nullptr; // Optional

        VULKAN_SUCCESS_ASSERT(vkQueuePresentKHR(_Data->Device.GetQueue(QueueFamilies::PRESENT), &presentInfo), "Queue Present Error");

        _Data->CurrentFrame = (_Data->CurrentFrame + 1) % _Spec.InFrameFlightCount;
    }

    void VulkanRenderApi::Begin(const RenderPassSpec &Spec)
    {
        vkWaitForFences(_Data->Device.GetHandle(), 1, &_Data->InFlightFences[_Data->CurrentFrame], VK_TRUE, UINT64_MAX);

        VkResult result = vkAcquireNextImageKHR(_Data->Device.GetHandle(), _Data->SwapChain, UINT64_MAX,
                                                _Data->ImageAvailableSemaphores[_Data->CurrentFrame], VK_NULL_HANDLE,
                                                &_Data->CurrentImageIndex);

        vkResetFences(_Data->Device.GetHandle(), 1, &_Data->InFlightFences[_Data->CurrentFrame]);
        vkResetCommandBuffer(_Data->GraphicsCommandBuffers[_Data->CurrentFrame], 0);

        auto commandBuffer = _Data->GraphicsCommandBuffers[_Data->CurrentFrame];

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        // 🆕 TRANSITION: undefined → color attachment optimal (for rendering)
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = _Data->SwapChainImages[_Data->CurrentImageIndex]; // 🆕 Use the actual image
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,             // Before any operations
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // Before color attachment output
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        // Continue with dynamic rendering...
        VkRenderingInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderingInfo.renderArea = {{0, 0}, _Data->Extent};
        renderingInfo.layerCount = 1;

        VkRenderingAttachmentInfo colorAttachment{};
        colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachment.imageView = _Data->SwapChainImageViews[_Data->CurrentImageIndex];
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // ✅ Now correct
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.clearValue = {{0.3f, 0.3f, 0.3f, 1.0f}};

        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachments = &colorAttachment;

        vkCmdBeginRendering(commandBuffer, &renderingInfo);
    }

    void VulkanRenderApi::End()
    {
        auto commandBuffer = _Data->GraphicsCommandBuffers[_Data->CurrentFrame];

        vkCmdEndRendering(commandBuffer);

        // 🆕 TRANSITION: color attachment optimal → present source (for presentation)
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // ✅ Required for presentation
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = _Data->SwapChainImages[_Data->CurrentImageIndex];
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask = 0;

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // After color attachment writing
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,          // Before presentation
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        vkEndCommandBuffer(commandBuffer);
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

        std::vector<VkPhysicalDevice> PDevices;
        PDevices.resize(PDeviceCount);
        vkEnumeratePhysicalDevices(_Data->Instance, &PDeviceCount, PDevices.data());

        if (PDevices.size() == 0)
            throw std::runtime_error("No Vulkan Supporting device found!");

        PRINTLN("[VULKAN]: Vulkan Supported devices: " << PDevices.size())
        _Data->PhysicalDevices.resize(PDevices.size());

        for (int i = 0; i < PDevices.size(); i++)
            _Data->PhysicalDevices[i].PhysicalDevice = PDevices[i];

        // Check how good each phsyical device is in features
        for (auto &device : _Data->PhysicalDevices)
        {
            _FillVulkanPhysicalDevice(device, _Data->SurfaceKHR);

            device.Info.SwapChainDetails = QuerySwapChainSupport(device.PhysicalDevice, _Data->SurfaceKHR);

            PRINTLN("[VULKAN]: Device Found: " << device.Info.properties.deviceName << "\n");
        }

        for (auto &device : _Data->PhysicalDevices)
            CheckVulkanPhysicalDeviceExtensions(device, _Spec.DeviceExtensions);

        _FindSuitablePhysicalDevice();
    }

    void VulkanRenderApi::_FindSuitablePhysicalDevice()
    {
        int Scores[_Data->PhysicalDevices.size()];

        int i = 0;
        for (auto &device : _Data->PhysicalDevices)
        {
            auto &deviceinfo = device.Info;
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
        for (int i = 0; i < _Data->PhysicalDevices.size(); i++)
        {
            if (Scores[Bestid] < Scores[i])
                Bestid = i;
        }
        PRINTLN("Chosen Device: " << _Data->PhysicalDevices[Bestid].Info.properties.deviceName << "\n")
        _Data->ActivePhysicalDeviceIndex = Bestid;
    }

    void VulkanRenderApi::_CreateLogicalDevice()
    {
        _Data->Device.Init(&_Data->PhysicalDevices[_Data->ActivePhysicalDeviceIndex], _Spec.DeviceExtensions);
    }

    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats)
    {
        for (const auto &availableFormat : availableFormats)
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                return availableFormat;

        // TODO: make a score system in future
        return availableFormats[0];
    }

    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes)
    {
        for (const auto &availablePresentMode : availablePresentModes)
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
                return availablePresentMode;

        // Always availale
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities, int FrameBufferWidth, int FrameBufferHeight)
    {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        {
            return capabilities.currentExtent;
        }
        else
        {
            VkExtent2D actualExtent = {
                static_cast<uint32_t>(FrameBufferWidth),
                static_cast<uint32_t>(FrameBufferHeight)};

            actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }

    void VulkanRenderApi::_CreateSwapChain()
    {
        auto &deviceInfo = _Data->PhysicalDevices[_Data->ActivePhysicalDeviceIndex].Info;
        SwapChainSupportDetails &support = deviceInfo.SwapChainDetails;

        // Choose surface format
        VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(support.formats);
        _Data->Format = surfaceFormat.format;

        // Choose present mode (prefer mailbox for triple buffering)
        VkPresentModeKHR presentMode = ChooseSwapPresentMode(support.presentModes);

        // Choose extent
        _Data->Extent = ChooseSwapExtent(support.capabilities, _Data->FrameBufferSize.x, _Data->FrameBufferSize.y);

        // Determine image count
        uint32_t imageCount = support.capabilities.minImageCount + 1;
        if (support.capabilities.maxImageCount > 0 && imageCount > support.capabilities.maxImageCount)
        {
            imageCount = support.capabilities.maxImageCount;
        }

        // Create swapchain
        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = _Data->SurfaceKHR;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = _Data->Extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        // Handle queue families
        QueueFamilyIndicies &indices = deviceInfo.QueueIndicies;
        uint32_t queueFamilyIndices[] = {indices.Queues[QueueFamilies::GRAPHICS].value(), indices.Queues[QueueFamilies::PRESENT].value()};

        if (indices.Queues[QueueFamilies::GRAPHICS].value() != indices.Queues[QueueFamilies::PRESENT].value())
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        createInfo.preTransform = support.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        VULKAN_SUCCESS_ASSERT(vkCreateSwapchainKHR(_Data->Device.GetHandle(), &createInfo, nullptr, &_Data->SwapChain), "[VULKAN]: SwapChain createion Failed!");
        PRINTLN("[VULKAN]: SwapChain created!");

        uint32_t count = 0;
        vkGetSwapchainImagesKHR(_Data->Device.GetHandle(), _Data->SwapChain, &count, nullptr);
        _Data->SwapChainImages.resize(count);
        vkGetSwapchainImagesKHR(_Data->Device.GetHandle(), _Data->SwapChain, &count, _Data->SwapChainImages.data());

        // Imave Views
        _Data->SwapChainImageViews.resize(_Data->SwapChainImages.size());

        for (int i = 0; i < _Data->SwapChainImages.size(); i++)
        {
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = _Data->SwapChainImages[i];
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = _Data->Format;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;
            viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

            VULKAN_SUCCESS_ASSERT(vkCreateImageView(_Data->Device.GetHandle(), &viewInfo, nullptr, &_Data->SwapChainImageViews[i]), "[VULKAN]: Image View Creation Failed!");
        }
        PRINTLN("[VULKAN]: Image Views created!");
    }

    void ReadFile(const char *FilePath, std::vector<char> &CharData)
    {
        std::ifstream file(FilePath, std::ios::ate | std::ios::binary);

        if (!file.is_open())
            throw std::runtime_error("failed to open file!");

        size_t fileSize = (size_t)file.tellg();
        CharData.resize(fileSize);

        file.seekg(0);
        file.read(CharData.data(), fileSize);
        file.close();
    }

    VkShaderModule CreateShaderModule(VkDevice device, std::vector<char> &code)
    {
        VkShaderModuleCreateInfo createinfo{};
        createinfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createinfo.codeSize = code.size();
        createinfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createinfo, nullptr, &shaderModule) != VK_SUCCESS)
            throw std::runtime_error("failed to create shader module!");

        return shaderModule;
    }

    VkPipelineShaderStageCreateInfo CreateShaderStages(VkShaderModule module, VkShaderStageFlagBits type, const char *Name)
    {
        VkPipelineShaderStageCreateInfo stage{};
        stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stage.module = module;
        stage.pName = Name;
        stage.stage = type;

        return stage;
    }

    void VulkanRenderApi::_CreateGraphiscPipeline()
    { // Load shaders
        auto device = _Data->Device.GetHandle();
        std::vector<char> VertShaderCode, FragShaderCode;
        ReadFile("vert.spv", VertShaderCode);
        ReadFile("frag.spv", FragShaderCode);

        VkShaderModule vertShader = CreateShaderModule(device, VertShaderCode);
        VkShaderModule fragShader = CreateShaderModule(device, FragShaderCode);

        // 🆕 NEW: Add dynamic rendering structure
        VkPipelineRenderingCreateInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachmentFormats = &_Data->Format; // Use swapchain format
        // Shader stages
        VkPipelineShaderStageCreateInfo shaderStages[] = {
            CreateShaderStages(vertShader, VK_SHADER_STAGE_VERTEX_BIT, "main"),
            CreateShaderStages(fragShader, VK_SHADER_STAGE_FRAGMENT_BIT, "main")};

        // Pipeline layout
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.pushConstantRangeCount = 0;    // Optional
        pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional
        vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &_Data->GraphcisPipelineLayout);

        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR};

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;

        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f; // Optional
        rasterizer.depthBiasClamp = 0.0f;          // Optional
        rasterizer.depthBiasSlopeFactor = 0.0f;    // Optional

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading = 1.0f;          // Optional
        multisampling.pSampleMask = nullptr;            // Optional
        multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
        multisampling.alphaToOneEnable = VK_FALSE;      // Optional

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;             // Optional
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;             // Optional

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f; // Optional
        colorBlending.blendConstants[1] = 0.0f; // Optional
        colorBlending.blendConstants[2] = 0.0f; // Optional
        colorBlending.blendConstants[3] = 0.0f; // Optional

        // Graphics pipeline
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

        VkPipelineVertexInputStateCreateInfo vertexInputinfo{};
        vertexInputinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputinfo.vertexBindingDescriptionCount = 0;
        vertexInputinfo.vertexAttributeDescriptionCount = 0;
        vertexInputinfo.pVertexAttributeDescriptions = nullptr;
        vertexInputinfo.pVertexBindingDescriptions = nullptr;

        pipelineInfo.pVertexInputState = &vertexInputinfo;
        pipelineInfo.pNext = &renderingInfo; // Link dynamic rendering info
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = nullptr; // Optional
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.layout = _Data->GraphcisPipelineLayout;
        pipelineInfo.pDepthStencilState = nullptr; // Optional
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineInfo.basePipelineIndex = -1;

        // Create pipeline!
        VULKAN_SUCCESS_ASSERT(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_Data->GraphcisPipeline), "Graphics pipeline Failed to create!");

        // Cleanup shaders
        vkDestroyShaderModule(device, vertShader, nullptr);
        vkDestroyShaderModule(device, fragShader, nullptr);
    }

    void VulkanRenderApi::_CreateCommandPool()
    {
        VkCommandPoolCreateInfo createinfo{};
        createinfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        createinfo.queueFamilyIndex = _Data->Device.GetPhysicalDevice()->Info.QueueIndicies.Queues[QueueFamilies::GRAPHICS].value();
        createinfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        VULKAN_SUCCESS_ASSERT(vkCreateCommandPool(_Data->Device.GetHandle(), &createinfo, nullptr, &_Data->GraphicsCommandPool), "Graphcis Command Pool Failed!");
        PRINTLN("[VULKAN]: Graphics Command Pool Created !!");
    }

    void VulkanRenderApi::_CreateCommandBuffer()
    {
        _Data->GraphicsCommandBuffers.resize(_Spec.InFrameFlightCount);

        VkCommandBufferAllocateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        info.commandPool = _Data->GraphicsCommandPool;
        info.commandBufferCount = _Spec.InFrameFlightCount;
        info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        VULKAN_SUCCESS_ASSERT(vkAllocateCommandBuffers(_Data->Device.GetHandle(), &info, _Data->GraphicsCommandBuffers.data()), "Command Buffer Failed!");
        PRINTLN("[VULKAN]: Graphics Command Buffers Created !!");
    }

    void VulkanRenderApi::_CreateSyncObjects()
    {
        _Data->ImageAvailableSemaphores.resize(_Spec.InFrameFlightCount);
        _Data->RenderFinishedSemaphores.resize(_Spec.InFrameFlightCount);
        _Data->InFlightFences.resize(_Spec.InFrameFlightCount);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Start signaled so first frame doesn't wait forever
        auto device = _Data->Device.GetHandle();

        for (size_t i = 0; i < _Spec.InFrameFlightCount; i++)
        {
            VULKAN_SUCCESS_ASSERT(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &_Data->ImageAvailableSemaphores[i]), "Image Available Sempahore Failed!");
            VULKAN_SUCCESS_ASSERT(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &_Data->RenderFinishedSemaphores[i]), "Render Finished Semaphore Failed!");
            VULKAN_SUCCESS_ASSERT(vkCreateFence(device, &fenceInfo, nullptr, &_Data->InFlightFences[i]), "Fences Failed To create!");
        }

        std::cout << "Created synchronization objects for " << _Spec.InFrameFlightCount << " frames in flight\n";
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
