#include "VeVPCH.h"
#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan.h"
#include "VulkanDevice.h"
#include <limits>
#include "VulkanRenderer.h"

#define PRINTLN(x)      \
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
    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    struct VulkanRendererData
    {
        VkInstance Instance;
        VkDebugUtilsMessengerEXT DebugMessenger;
        int ActivePhysicalDeviceIndex;
        int ActiveDeviceIndex;
        QueueFamilyIndices QueueIndicies;
        VkSurfaceKHR Surface;
        VkSwapchainKHR SwapChain;
        std::vector<VkImage> SwapChainImages;
        std::vector<VkImageView> SwapChainImageViews;
        std::vector<VkFramebuffer> SwapChainFrameBuffers;
        VkCommandPool CommandPool;
        VkCommandBuffer CommandBuffer;

        VkFormat SwapChainImageFormat;
        VkExtent2D SwapChainExtent;
        VkRenderPass RenderPass;

        // Abstract to classes (RAII)
        std::vector<VulkanPhysicalDevice> PhysicalDevices;
        std::vector<VulkanDevice> Devices;
        VulkanPhysicalDevice *ActivePhysicalDevice;
        VulkanDevice *ActiveDevice;
        VkPipelineLayout PipelineLayout;
        VkSemaphore imageAvailableSemaphore;
        VkSemaphore renderFinishedSemaphore;
        VkFence inFlightFence;

        VkPipeline GraphicsPipeline;
    };

    QueueFamilyIndices _FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR Surface)
    {
        uint32_t Count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &Count, nullptr);

        std::vector<VkQueueFamilyProperties> Properties(Count);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &Count, Properties.data());
        QueueFamilyIndices Indicies;

        int i = 0;
        for (const auto &queueFamily : Properties)
        {
            VkBool32 presentSupport = false;
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                Indicies.Queues[QueueFamilies::GRAPHICS] = i;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, Surface, &presentSupport);

            if (presentSupport)
                Indicies.Queues[QueueFamilies::PRESENT] = i;
            i++;
        }
        return Indicies;
    }

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
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

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats)
    {
        for (const auto &availableFormat : availableFormats)
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                return availableFormat;

        // TODO: make a score system in future
        return availableFormats[0];
    }

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes)
    {
        for (const auto &availablePresentMode : availablePresentModes)
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
                return availablePresentMode;

        // Always availale
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities, int FrameBufferWidth, int FrameBufferHeight)
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

    void VulkanRenderer::_RecordCommandBuffer(uint32_t imageIndex)
    {
        auto commandBuffer = _Data->CommandBuffer;
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;                  // Optional
        beginInfo.pInheritanceInfo = nullptr; // Optional

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
            throw std::runtime_error("failed to begin recording command buffer!");

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = _Data->RenderPass;
        renderPassInfo.framebuffer = _Data->SwapChainFrameBuffers[imageIndex];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = _Data->SwapChainExtent;

        VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _Data->GraphicsPipeline);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(_Data->SwapChainExtent.width);
        viewport.height = static_cast<float>(_Data->SwapChainExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = _Data->SwapChainExtent;
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        vkCmdDraw(commandBuffer, 3, 1, 0, 0);

        vkCmdEndRenderPass(commandBuffer);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
            throw std::runtime_error("failed to record command buffer!");
    }

    void VulkanRenderer::_CreateSyncObject()
    {
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        if (vkCreateSemaphore(_Data->ActiveDevice->GetHandle(), &semaphoreInfo, nullptr, &_Data->imageAvailableSemaphore) != VK_SUCCESS ||
            vkCreateSemaphore(_Data->ActiveDevice->GetHandle(), &semaphoreInfo, nullptr, &_Data->renderFinishedSemaphore) != VK_SUCCESS ||
            vkCreateFence(_Data->ActiveDevice->GetHandle(), &fenceInfo, nullptr, &_Data->inFlightFence) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create semaphores!");
        }
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

    VulkanRenderer::VulkanRenderer()
    {
    }

    VulkanRenderer::~VulkanRenderer()
    {
        delete _Data;
    }

    void VulkanRenderer::Terminate()
    {
        vkDeviceWaitIdle(_Data->ActiveDevice->GetHandle());

        vkDestroySemaphore(_Data->ActiveDevice->GetHandle(), _Data->imageAvailableSemaphore, nullptr);
        vkDestroySemaphore(_Data->ActiveDevice->GetHandle(), _Data->renderFinishedSemaphore, nullptr);
        vkDestroyFence(_Data->ActiveDevice->GetHandle(), _Data->inFlightFence, nullptr);

        vkDestroyCommandPool(_Data->ActiveDevice->GetHandle(), _Data->CommandPool, nullptr);
        for (auto framebuffer : _Data->SwapChainFrameBuffers)
            vkDestroyFramebuffer(_Data->ActiveDevice->GetHandle(), framebuffer, nullptr);

        vkDestroyPipeline(_Data->ActiveDevice->GetHandle(), _Data->GraphicsPipeline, nullptr);
        vkDestroyPipelineLayout(_Data->ActiveDevice->GetHandle(), _Data->PipelineLayout, nullptr);
        vkDestroyRenderPass(_Data->ActiveDevice->GetHandle(), _Data->RenderPass, nullptr);

        for (auto imageView : _Data->SwapChainImageViews)
            vkDestroyImageView(_Data->ActiveDevice->GetHandle(), imageView, nullptr);

        vkDestroySwapchainKHR(_Data->ActiveDevice->GetHandle(), _Data->SwapChain, nullptr);

        for (auto Device : _Data->Devices)
            Device.Destroy();

        if (_Spec.EnableValidationLayer)
            DestroyDebugUtilsMessengerEXT(_Data->Instance, _Data->DebugMessenger, nullptr);

        vkDestroySurfaceKHR(_Data->Instance, _Data->Surface, nullptr);

        vkDestroyInstance(_Data->Instance, nullptr);
        PRINTLN("Terminated vulkan!")
    }

    void VulkanRenderer::Render()
    {
        vkWaitForFences(_Data->ActiveDevice->GetHandle(), 1, &_Data->inFlightFence, VK_TRUE, UINT64_MAX);
        vkResetFences(_Data->ActiveDevice->GetHandle(), 1, &_Data->inFlightFence);

        uint32_t imageIndex;
        vkAcquireNextImageKHR(_Data->ActiveDevice->GetHandle(), _Data->SwapChain, UINT64_MAX, _Data->imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
        vkResetCommandBuffer(_Data->CommandBuffer, 0);
        _RecordCommandBuffer(imageIndex);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = {_Data->imageAvailableSemaphore};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &_Data->CommandBuffer;

        VkSemaphore signalSemaphores[] = {_Data->renderFinishedSemaphore};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(_Data->ActiveDevice->GetQueues(QueueFamilies::GRAPHICS), 1, &submitInfo, _Data->inFlightFence) != VK_SUCCESS)
            throw std::runtime_error("failed to submit draw command buffer!");

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = {_Data->SwapChain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;
        presentInfo.pResults = nullptr; // Optional

        vkQueuePresentKHR(_Data->ActiveDevice->GetQueues(QueueFamilies::PRESENT), &presentInfo);

    }

    void VulkanRenderer::Init(void *Spec)
    {
        _Data = new VulkanRendererData();
        _Spec = *(VulkanRenderSpec *)Spec;

        // For Swapchain
        _Spec.DeviceRequirerdExtensions.push_back("VK_KHR_swapchain");

        _CreateInstance();
        _CreateDebugMessenger();
        _CreateWindowSurface();
        _CreateSuitablePhysicalDevice();
        _CreateSuitableLogicalDevice();
        _CreateSwapChain();
        _CreateImageViews();
        _CreateRenderPass();
        _CreateGraphicsPipeline();
        _CreateFrameBuffers();
        _CreateCommandPool();
        _CreateCommandBuffer();
        _CreateSyncObject();
    }

    void VulkanRenderer::_CreateInstance()
    {
        if (_Spec.EnableValidationLayer)
        {
            // _Spec.DeviceRequirerdExtensions.push_back("VK_LAYER_KHRONOS_validation");
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
            VulkanDeviceSpec Spec;
            Spec.PhysicalDevice = &Device;
            Spec.RequiredExtensions = _Spec.DeviceRequirerdExtensions;

            _Data->Devices.push_back(VulkanDevice());
            _Data->Devices[i].Init(Spec);

            PRINTLN("[VULKAN]: Logical Device Created! of: " << Device.GetProps().Name)

            i++;
        }
        _Data->ActiveDeviceIndex = _Data->ActivePhysicalDeviceIndex;
        _Data->ActiveDevice = &_Data->Devices[_Data->ActiveDeviceIndex];
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

    void VulkanRenderer::_CreateSwapChain()
    {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(_Data->ActivePhysicalDevice->GetHandle(), _Data->Surface);

        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities, _Spec.FrameBufferWidth, _Spec.FrameBufferHeight);

        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
            imageCount = swapChainSupport.capabilities.maxImageCount;

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = _Data->Surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        QueueFamilyIndices indices = _Data->ActivePhysicalDevice->GetQueueFamilyIndicies();
        uint32_t queueFamilyIndices[] = {indices.Queues[QueueFamilies::GRAPHICS].value(), indices.Queues[QueueFamilies::PRESENT].value()};

        if (indices.Queues[QueueFamilies::GRAPHICS] != indices.Queues[QueueFamilies::PRESENT])
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;     // Optional
            createInfo.pQueueFamilyIndices = nullptr; // Optional
        }
        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;

        createInfo.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(_Data->ActiveDevice->GetHandle(), &createInfo, nullptr, &_Data->SwapChain) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create swap chain!");
        }

        vkGetSwapchainImagesKHR(_Data->ActiveDevice->GetHandle(), _Data->SwapChain, &imageCount, nullptr);
        _Data->SwapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(_Data->ActiveDevice->GetHandle(), _Data->SwapChain, &imageCount, _Data->SwapChainImages.data());

        _Data->SwapChainImageFormat = surfaceFormat.format;
        _Data->SwapChainExtent = extent;

        PRINTLN("[VULKAN]: SwapChain Created")
    }

    void VulkanRenderer::_CreateImageViews()
    {
        _Data->SwapChainImageViews.resize(_Data->SwapChainImages.size());

        for (size_t i = 0; i < _Data->SwapChainImages.size(); i++)
        {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = _Data->SwapChainImages[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = _Data->SwapChainImageFormat;

            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(_Data->ActiveDevice->GetHandle(), &createInfo, nullptr, &_Data->SwapChainImageViews[i]) != VK_SUCCESS)
                throw std::runtime_error("failed to create image views!");
            PRINTLN("[VULKAN]: ImageView Created!")
        }
    }

    VkShaderModule createShaderModule(VkDevice device, const std::vector<char> &code)
    {
        VkShaderModuleCreateInfo createinfo;
        createinfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createinfo.codeSize = code.size();
        createinfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createinfo, nullptr, &shaderModule) != VK_SUCCESS)
            throw std::runtime_error("failed to create shader module!");
        return shaderModule;
    }

    void VulkanRenderer::_CreateGraphicsPipeline()
    {
        std::vector<char> vertexsource, fragsource;
        ReadFile("vert.spv", vertexsource);
        ReadFile("frag.spv", fragsource);

        VkShaderModule vertShaderModule = createShaderModule(_Data->ActiveDevice->GetHandle(), vertexsource);
        VkShaderModule fragShaderModule = createShaderModule(_Data->ActiveDevice->GetHandle(), fragsource);

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};
        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR};

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 0;
        vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
        vertexInputInfo.vertexAttributeDescriptionCount = 0;
        vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)_Data->SwapChainExtent.width;
        viewport.height = (float)_Data->SwapChainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = _Data->SwapChainExtent;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;

        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

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

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};

        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0;            // Optional
        pipelineLayoutInfo.pSetLayouts = nullptr;         // Optional
        pipelineLayoutInfo.pushConstantRangeCount = 0;    // Optional
        pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

        if (vkCreatePipelineLayout(_Data->ActiveDevice->GetHandle(), &pipelineLayoutInfo, nullptr, &_Data->PipelineLayout) != VK_SUCCESS)
            throw std::runtime_error("failed to create pipeline layout!");

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = nullptr; // Optional
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = _Data->PipelineLayout;
        pipelineInfo.renderPass = _Data->RenderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
        pipelineInfo.basePipelineIndex = -1;              // Optional

        if (vkCreateGraphicsPipelines(_Data->ActiveDevice->GetHandle(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_Data->GraphicsPipeline) != VK_SUCCESS)
            throw std::runtime_error("failed to create graphics pipeline!");

        PRINTLN("[VULKAN]: Graphics Pipeline Created!")

        vkDestroyShaderModule(_Data->ActiveDevice->GetHandle(), fragShaderModule, nullptr);
        vkDestroyShaderModule(_Data->ActiveDevice->GetHandle(), vertShaderModule, nullptr);
    }

    void VulkanRenderer::_CreateFrameBuffers()
    {
        _Data->SwapChainFrameBuffers.resize(_Data->SwapChainImageViews.size());
        for (size_t i = 0; i < _Data->SwapChainImageViews.size(); i++)
        {
            VkImageView attachments[] = {
                _Data->SwapChainImageViews[i]};

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = _Data->RenderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = _Data->SwapChainExtent.width;
            framebufferInfo.height = _Data->SwapChainExtent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(_Data->ActiveDevice->GetHandle(), &framebufferInfo, nullptr, &_Data->SwapChainFrameBuffers[i]) != VK_SUCCESS)
                throw std::runtime_error("failed to create framebuffer!");
        }
    }

    void VulkanRenderer::_CreateCommandPool()
    {
        QueueFamilyIndices queueFamilyIndices = _Data->ActivePhysicalDevice->GetQueueFamilyIndicies();

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = queueFamilyIndices.Queues[QueueFamilies::GRAPHICS].value();

        if (vkCreateCommandPool(_Data->ActiveDevice->GetHandle(), &poolInfo, nullptr, &_Data->CommandPool) != VK_SUCCESS)
            throw std::runtime_error("failed to create command pool!");
    }

    void VulkanRenderer::_CreateCommandBuffer()
    {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = _Data->CommandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(_Data->ActiveDevice->GetHandle(), &allocInfo, &_Data->CommandBuffer) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }

    void VulkanRenderer::_PopulatePhysicalDevices()
    {
        PopulateVulkanPhysicalDevice(_Data->PhysicalDevices, _Data->Instance, _Data->Surface);
    }

    void VulkanRenderer::_CheckSuitablePhysicalDevice()
    {
        int Scores[_Data->PhysicalDevices.size()] = {0};

        int i = 0;

        for (auto Device : _Data->PhysicalDevices)
        {
            PRINTLN("Devices:    ")
            auto Indicies = Device.GetQueueFamilyIndicies();
            auto Props = Device.GetProps();

            Device.PrintInfo();
            if (Props.DeviceType == VulkanPhysicalDeviceType::DISCRETE)
                Scores[i] += 1000;

            Scores[i] += Device.GetLimits().maxImageDimension2D;

            if (Indicies.Queues[QueueFamilies::GRAPHICS].has_value())
                Scores[i] += 10000;

            if (!Device.GetFeatures().geometryShader)
                Scores[i] = 0;

            if (!Device.CheckExtensionsSupported(_Spec.DeviceRequirerdExtensions))
                Scores[i] = 0;
            else
            {
                bool swapChainAdequate = false;

                SwapChainSupportDetails swapChainSupport = querySwapChainSupport(Device.GetHandle(), _Data->Surface);
                swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
            }
            i++;
            PRINTLN("           ")
        }

        int Bestid = 0;
        for (int i = 0; i < _Data->PhysicalDevices.size(); i++)
        {
            if (Scores[Bestid] < Scores[i])
                Bestid = i;
        }
        _Data->ActivePhysicalDeviceIndex = Bestid;
        _Data->ActivePhysicalDevice = &_Data->PhysicalDevices[_Data->ActivePhysicalDeviceIndex];

        PRINTLN("Chosen Device:    ")
        _Data->ActivePhysicalDevice->PrintInfo();
        PRINTLN("           ")
    }

    void VulkanRenderer::_CreateRenderPass()
    {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = _Data->SwapChainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(_Data->ActiveDevice->GetHandle(), &renderPassInfo, nullptr, &_Data->RenderPass) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create render pass!");
        }
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
