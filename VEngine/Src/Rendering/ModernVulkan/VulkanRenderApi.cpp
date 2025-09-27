#include "VeVPCH.h"
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan/vulkan.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"
#include "VulkanRenderApi.h"
#include "VulkanDevice.h"
#include "VulkanResourceFactory.h"
#include "VulkanRenderData.h"
#include "VulkanUtils.h"

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

#pragma region VulkanRenderApi
    void VulkanRenderApi::Init(void *Spec)
    {
        auto SupportedVersion = VulkanUtils::GetInstanceVulkanVersion();
        PRINTLN("[VULAKN]: Supports: " << SupportedVersion.Max << "." << SupportedVersion.Min << "." << SupportedVersion.Patch);
        if (SupportedVersion.Min < 3)
        {
            throw std::runtime_error("Vulkan Version not supported!");
        }

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
        _CreateUniformBuffers();

        _CreateDescriptorSetLayout();
        _CreateDescriptorPool();
        _CreateDescriptorSets();

        _CreateCommandPool();
        _CreateCommandBuffer();
        _CreateSyncObjects();

        _ResourceFactory = std::make_shared<VulkanResourceFactory>(_Data);
        PRINTLN("[VULKAN]: Vulkan Resource Factory Api Created!!");

        _CreateTextures();
        _CreateTextureDescriptorResources();
        _CreateDepthData();

        _CreateGraphiscPipeline();

        PRINTLN("[VULKAN]: New Modern Render Api Created!!");
    }

    void VulkanRenderApi::Terminate()
    {
        vkDeviceWaitIdle(_Data->Device.GetHandle());
        _ResourceFactory->Terminate();
        PRINTLN("[VULKAN]: Vulkan Resource Factory Api Terminated!!");

        vkDestroyDescriptorPool(_Data->Device.GetHandle(), _Data->TextureDescriptor.pool, nullptr);
        vkDestroyDescriptorSetLayout(_Data->Device.GetHandle(), _Data->TextureDescriptor.layout, nullptr);

        vkDestroyImageView(_Data->Device.GetHandle(), _Data->DepthData.imageview, nullptr);
        vmaDestroyImage(_Data->Allocator, _Data->DepthData.image, _Data->DepthData.allocation);

        vkDestroySampler(_Data->Device.GetHandle(), _Data->Texture.sampler, nullptr);
        vkDestroyImageView(_Data->Device.GetHandle(), _Data->Texture.imageView, nullptr);
        vmaDestroyImage(_Data->Allocator, _Data->Texture.image, _Data->Texture.allocation);
        _Data->GraphicsPipeline.Destroy(_Data->Device.GetHandle());

        for (int i = 0; i < _Spec.InFrameFlightCount; i++)
            _Data->UniformBuffers[i]->Destroy(_Data->Allocator);

        vkDestroyDescriptorPool(_Data->Device.GetHandle(), _Data->DescriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(_Data->Device.GetHandle(), _Data->DescriptorSetLayout, nullptr);

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
        vkDestroyCommandPool(_Data->Device.GetHandle(), _Data->TransferCommandPool, nullptr);

        _DestroySwapChain();

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

    void VulkanRenderApi::FrameBufferResize(int x, int y)
    {
        _Data->FrameBufferSize.x = x;
        _Data->FrameBufferSize.y = y;
        _Data->Extent.width = x;
        _Data->Extent.height = y;
        _Data->FrameBufferChanged = true;
    }

    void VulkanRenderApi::Submit(const Ref<VertexBuffer> &VB, const Ref<IndexBuffer> &IB)
    {
        _UploadUniformBuffer();
        auto VulkanVB = std::static_pointer_cast<VulkanVertexBuffer>(VB);
        auto VulkanIB = std::static_pointer_cast<VulkanIndexBuffer>(IB);

        auto commandBuffer = _Data->GraphicsCommandBuffers[_Data->CurrentFrame];
        // Bind pipeline (created without render pass)
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _Data->GraphicsPipeline.GetHandle());

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

        // 🆕 Bind vertex buffer
        VkBuffer vertexBuffers[] = {VulkanVB->GetHandle()};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _Data->GraphicsPipeline.GetLayout(), 0, 1, &_Data->TextureDescriptor.sets[_Data->CurrentFrame], 0, nullptr);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _Data->GraphicsPipeline.GetLayout(), 1, 1, &_Data->DescriptorSets[_Data->CurrentFrame], 0, nullptr);

        vkCmdBindIndexBuffer(commandBuffer, VulkanIB->GetHandle(), 0, VK_INDEX_TYPE_UINT16);
        // 🎨 Draw your geometry
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(VulkanIB->GetCount()), 1, 0, 0, 0);
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

        auto result = vkQueuePresentKHR(_Data->Device.GetQueue(QueueFamilies::PRESENT), &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || _Data->FrameBufferChanged)
        {
            _Data->FrameBufferChanged = false;
            _RecreateSwapChain();
        }
        else if (result != VK_SUCCESS)
            throw std::runtime_error("failed to present swap chain image!");

        _Data->CurrentFrame = (_Data->CurrentFrame + 1) % _Spec.InFrameFlightCount;
    }

    void VulkanRenderApi::Begin(const RenderPassSpec &Spec)
    {
        vkWaitForFences(_Data->Device.GetHandle(), 1, &_Data->InFlightFences[_Data->CurrentFrame], VK_TRUE, UINT64_MAX);

        VkResult result = vkAcquireNextImageKHR(_Data->Device.GetHandle(), _Data->SwapChain, UINT64_MAX,
                                                _Data->ImageAvailableSemaphores[_Data->CurrentFrame], VK_NULL_HANDLE,
                                                &_Data->CurrentImageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            _RecreateSwapChain();
            return;
        }
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
            throw std::runtime_error("failed to acquire swap chain image!");

        if (result != VK_SUCCESS)
        {
            std::cout << "ERROR: AcquireNextImage failed: " << result << "\n";
        }
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
        colorAttachment.clearValue = {{Spec.ClearColor.x, Spec.ClearColor.y, Spec.ClearColor.z, Spec.ClearColor.w}};

        // Depth attachment  ✅ ADD THIS
        VkRenderingAttachmentInfo depthAttachment{};
        depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depthAttachment.imageView = _Data->DepthData.imageview;
        depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.clearValue.depthStencil = {1.0f, 0}; // Clear to far plane

        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachments = &colorAttachment;
        renderingInfo.pDepthAttachment = &depthAttachment; // ✅ ADD THIS

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

    void VulkanRenderApi::Finish()
    {
        vkDeviceWaitIdle(_Data->Device.GetHandle());
    }

    Ref<ResourceFactory> VulkanRenderApi::GetResourceFactory()
    {
        return _ResourceFactory;
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
            imageCount = support.capabilities.maxImageCount;

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

    void VulkanRenderApi::_RecreateSwapChain()
    {
        vkDeviceWaitIdle(_Data->Device.GetHandle());
        _DestroySwapChain();
        _CreateSwapChain();
    }

    void VulkanRenderApi::_DestroySwapChain()
    {
        for (auto imageview : _Data->SwapChainImageViews)
            vkDestroyImageView(_Data->Device.GetHandle(), imageview, nullptr);
        vkDestroySwapchainKHR(_Data->Device.GetHandle(), _Data->SwapChain, nullptr);
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
    {
        VulkanGraphicsPipelineSpec GraphicsSpec;
        GraphicsSpec.Attributes = {{0, 0, ShaderDataType::FLOAT3, "position"}, {0, 1, ShaderDataType::FLOAT3, "color"}, {0, 2, ShaderDataType::FLOAT2, "tex"}};
        GraphicsSpec.Paths = {"vert.spv", "frag.spv"};
        GraphicsSpec.Name = "main";
        GraphicsSpec.device = _Data->Device.GetHandle();
        GraphicsSpec.SwapChainFormat = _Data->Format;
        GraphicsSpec.DescLayouts = {_Data->TextureDescriptor.layout, _Data->DescriptorSetLayout};
        GraphicsSpec.UseDepth = true;
        GraphicsSpec.DepthFormat = _Data->DepthData.format;

        _Data->GraphicsPipeline.Init(GraphicsSpec);
    }

    void VulkanRenderApi::_CreateCommandPool()
    {
        VkCommandPoolCreateInfo graphicscreateinfo{};
        graphicscreateinfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        graphicscreateinfo.queueFamilyIndex = _Data->Device.GetPhysicalDevice()->Info.QueueIndicies.Queues[QueueFamilies::GRAPHICS].value();
        graphicscreateinfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        VULKAN_SUCCESS_ASSERT(vkCreateCommandPool(_Data->Device.GetHandle(), &graphicscreateinfo, nullptr, &_Data->GraphicsCommandPool), "Graphcis Command Pool Failed!");
        PRINTLN("[VULKAN]: Graphics Command Pool Created !!");

        VkCommandPoolCreateInfo transfercreateinfo{};
        transfercreateinfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        transfercreateinfo.queueFamilyIndex = _Data->Device.GetPhysicalDevice()->Info.QueueIndicies.Queues[QueueFamilies::TRANSFER].value();
        transfercreateinfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        VULKAN_SUCCESS_ASSERT(vkCreateCommandPool(_Data->Device.GetHandle(), &transfercreateinfo, nullptr, &_Data->TransferCommandPool), "Graphcis Command Pool Failed!");
        PRINTLN("[VULKAN]: Transfer Command Pool Created !!");
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

    void VulkanRenderApi::_CreateDescriptorSetLayout()
    {
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 1;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &uboLayoutBinding;

        if (vkCreateDescriptorSetLayout(_Data->Device.GetHandle(), &layoutInfo, nullptr, &_Data->DescriptorSetLayout) != VK_SUCCESS)
            throw std::runtime_error("failed to create descriptor set layout!");
    }

    void VulkanRenderApi::_CreateUniformBuffers()
    {
        _Data->UniformBuffers.resize(_Spec.InFrameFlightCount);
        for (int i = 0; i < _Spec.InFrameFlightCount; i++)
        {
            UniformBufferDesc desc{};
            desc.Size = sizeof(UniformBufferObject);
            _Data->UniformBuffers[i] = std::make_unique<VulkanUniformBuffer>(_Data->Allocator, desc);
        }
    }

    void VulkanRenderApi::_UploadUniformBuffer()
    {
        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
        UniformBufferObject ubo{};
        ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.proj = glm::perspective(glm::radians(45.0f), _Data->FrameBufferSize.x / (float)_Data->FrameBufferSize.y, 0.1f, 10.0f);
        ubo.proj[1][1] *= -1;

        _Data->UniformBuffers[_Data->CurrentFrame]->UploadData(&ubo, sizeof(ubo));
    }

    void VulkanRenderApi::_CreateDescriptorPool()
    {
        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize.descriptorCount = static_cast<uint32_t>(_Spec.InFrameFlightCount);

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;
        poolInfo.maxSets = static_cast<uint32_t>(_Spec.InFrameFlightCount);

        if (vkCreateDescriptorPool(_Data->Device.GetHandle(), &poolInfo, nullptr, &_Data->DescriptorPool) != VK_SUCCESS)
            throw std::runtime_error("failed to create descriptor pool!");
    }

    void VulkanRenderApi::_CreateDescriptorSets()
    {
        std::vector<VkDescriptorSetLayout> layouts(_Spec.InFrameFlightCount, _Data->DescriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = _Data->DescriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(_Spec.InFrameFlightCount);
        allocInfo.pSetLayouts = layouts.data();

        _Data->DescriptorSets.resize(_Spec.InFrameFlightCount);
        if (vkAllocateDescriptorSets(_Data->Device.GetHandle(), &allocInfo, _Data->DescriptorSets.data()) != VK_SUCCESS)
            throw std::runtime_error("failed to allocate descriptor sets!");

        for (size_t i = 0; i < _Spec.InFrameFlightCount; i++)
        {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = _Data->UniformBuffers[i]->GetHandle();
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            VkWriteDescriptorSet descriptorWrite{};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = _Data->DescriptorSets[i];
            descriptorWrite.dstBinding = 1;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.pBufferInfo = &bufferInfo;
            descriptorWrite.pImageInfo = nullptr;       // Optional
            descriptorWrite.pTexelBufferView = nullptr; // Optional

            vkUpdateDescriptorSets(_Data->Device.GetHandle(), 1, &descriptorWrite, 0, nullptr);
        }
    }

    struct VulkanImageSpec
    {
        struct
        {
            int x, y;
        } Dims;
        VkImageType ImageType;
        VkFormat Format;
        VkImageLayout Layout;
        VkImageUsageFlags Usage;
        VkSharingMode SharingMode;
        VkSampleCountFlagBits Samples;
        VkImageTiling Tiling;
    };

    void CreateVulkanImage(VmaAllocator Allocator, VkImage &image, VmaAllocation &Allocation, const VulkanImageSpec &Spec)
    {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.extent.width = Spec.Dims.x;
        imageInfo.extent.height = Spec.Dims.y;
        imageInfo.extent.depth = 1;
        imageInfo.imageType = Spec.ImageType;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = Spec.Format;
        imageInfo.tiling = Spec.Tiling;
        imageInfo.initialLayout = Spec.Layout;
        imageInfo.usage = Spec.Usage;
        imageInfo.sharingMode = Spec.SharingMode;
        imageInfo.samples = Spec.Samples;

        VmaAllocationCreateInfo imageAllocInfo{};
        imageAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        imageAllocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        vmaCreateImage(Allocator, &imageInfo, &imageAllocInfo, &image, &Allocation, nullptr);
    }

    void VulkanRenderApi::_CreateTextures()
    {
        const char *TexFilePath = "images.png";

        // Load image data using stb_image
        int texWidth, texHeight, texChannels;
        stbi_uc *pixels = stbi_load(TexFilePath, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

        if (!pixels)
            throw std::runtime_error("failed to load texture image!");

        _Data->Texture.height = texHeight;
        _Data->Texture.width = texWidth;
        _Data->Texture.format = VK_FORMAT_R8G8B8A8_SRGB;

        VkDeviceSize imageSize = texWidth * texHeight * 4;

        VulkanStageBufferSpec StagingBufferSpec;
        StagingBufferSpec.Data = (void *)pixels;
        StagingBufferSpec.Size = imageSize;
        VulkanStageBuffer StagingBuffer(_Data->Allocator, StagingBufferSpec);

        stbi_image_free(pixels);

        VulkanImageSpec ImageSpec{};
        ImageSpec.Dims.x = texWidth;
        ImageSpec.Dims.y = texHeight;
        ImageSpec.Format = VK_FORMAT_R8G8B8A8_SRGB;
        ImageSpec.ImageType = VK_IMAGE_TYPE_2D;
        ImageSpec.Layout = VK_IMAGE_LAYOUT_UNDEFINED;
        ImageSpec.Samples = VK_SAMPLE_COUNT_1_BIT;
        ImageSpec.Usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        ImageSpec.SharingMode = VK_SHARING_MODE_EXCLUSIVE;
        ImageSpec.Tiling = VK_IMAGE_TILING_OPTIMAL;
        CreateVulkanImage(_Data->Allocator, _Data->Texture.image, _Data->Texture.allocation, ImageSpec);

        SingleTimeCommandBuffer SingleCommandSpec{};
        SingleCommandSpec.Family = QueueFamilies::TRANSFER;

        // Make image able to recieve data from gpu
        _ResourceFactory->BeginSingleTimeCommand(SingleCommandSpec);
        _TransitionImageLayout(SingleCommandSpec.Cmd, _Data->Texture.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        _ResourceFactory->EndSingleTimeCommand(SingleCommandSpec);

        // Copy buffer to image
        _ResourceFactory->BeginSingleTimeCommand(SingleCommandSpec);

        // Define region to copy
        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;   // Tightly packed
        region.bufferImageHeight = 0; // Tightly packed

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;

        region.imageOffset = {0, 0, 0};
        region.imageExtent = {_Data->Texture.width, _Data->Texture.height, 1};

        // Issue copy command
        vkCmdCopyBufferToImage(
            SingleCommandSpec.Cmd,
            StagingBuffer.GetHandle(),
            _Data->Texture.image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region);
        _ResourceFactory->EndSingleTimeCommand(SingleCommandSpec);

        // Make image able to be read by shader
        _ResourceFactory->BeginSingleTimeCommand(SingleCommandSpec);
        _TransitionImageLayout(SingleCommandSpec.Cmd, _Data->Texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        _ResourceFactory->EndSingleTimeCommand(SingleCommandSpec);

        _Data->Texture.imageView = _CreateTextureImageView(_Data->Texture.image, _Data->Texture.format, VK_IMAGE_ASPECT_COLOR_BIT);
        _Data->Texture.sampler = _CreateTextureSampler();

        StagingBuffer.Destroy(_Data->Allocator);
    }

    VkImageView VulkanRenderApi::_CreateTextureImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
    {
        VkImageView ImageView;

        VkImageViewCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        info.format = format;
        info.image = image;
        info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        info.subresourceRange.aspectMask = aspectFlags;
        info.subresourceRange.baseMipLevel = 0;
        info.subresourceRange.levelCount = 1;
        info.subresourceRange.baseArrayLayer = 0;
        info.subresourceRange.layerCount = 1;

        VULKAN_SUCCESS_ASSERT(vkCreateImageView(_Data->Device.GetHandle(), &info, nullptr, &ImageView), "[VULKAN]: Image View failed!");

        return ImageView;
    }

    VkSampler VulkanRenderApi::_CreateTextureSampler()
    {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_TRUE;

        samplerInfo.maxAnisotropy = _Data->PhysicalDevices[_Data->ActivePhysicalDeviceIndex].Info.properties.limits.maxSamplerAnisotropy;

        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;

        VkSampler sampler;
        if (vkCreateSampler(_Data->Device.GetHandle(), &samplerInfo, nullptr, &sampler) != VK_SUCCESS)
            throw std::runtime_error("failed to create texture sampler!");

        return sampler;
    }

    void VulkanRenderApi::_CreateTextureDescriptorResources()
    {
        // Create descriptor set layout for texture
        VkDescriptorSetLayoutBinding samplerLayoutBinding{};
        samplerLayoutBinding.binding = 0;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.pImmutableSamplers = nullptr;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &samplerLayoutBinding;

        if (vkCreateDescriptorSetLayout(_Data->Device.GetHandle(), &layoutInfo, nullptr, &_Data->TextureDescriptor.layout) != VK_SUCCESS)
            throw std::runtime_error("failed to create texture descriptor set layout!");
        // Create descriptor pool
        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        poolSize.descriptorCount = static_cast<uint32_t>(_Spec.InFrameFlightCount);

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;
        poolInfo.maxSets = static_cast<uint32_t>(_Spec.InFrameFlightCount);

        if (vkCreateDescriptorPool(_Data->Device.GetHandle(), &poolInfo, nullptr, &_Data->TextureDescriptor.pool) != VK_SUCCESS)
            throw std::runtime_error("failed to create texture descriptor pool!");

        // Allocate descriptor sets
        std::vector<VkDescriptorSetLayout> layouts(_Spec.InFrameFlightCount, _Data->TextureDescriptor.layout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = _Data->TextureDescriptor.pool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(_Spec.InFrameFlightCount);
        allocInfo.pSetLayouts = layouts.data();

        _Data->TextureDescriptor.sets.resize(_Spec.InFrameFlightCount);
        if (vkAllocateDescriptorSets(_Data->Device.GetHandle(), &allocInfo, _Data->TextureDescriptor.sets.data()) != VK_SUCCESS)
            throw std::runtime_error("failed to allocate texture descriptor sets!");

        for (int i = 0; i < _Spec.InFrameFlightCount; i++)
        {
            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = _Data->Texture.imageView;
            imageInfo.sampler = _Data->Texture.sampler;

            VkWriteDescriptorSet descriptorWrite{};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = _Data->TextureDescriptor.sets[i];
            descriptorWrite.dstBinding = 0;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.pImageInfo = &imageInfo;

            vkUpdateDescriptorSets(_Data->Device.GetHandle(), 1, &descriptorWrite, 0, nullptr);
        }
    }

    void VulkanRenderApi::_CreateDepthData()
    {
        _Data->DepthData.format = FindDepthFormat();

        VulkanImageSpec ImageSpec{};
        ImageSpec.Dims.x = _Data->FrameBufferSize.x;
        ImageSpec.Dims.y = _Data->FrameBufferSize.y;
        ImageSpec.Format = _Data->DepthData.format;
        ImageSpec.ImageType = VK_IMAGE_TYPE_2D;
        ImageSpec.Layout = VK_IMAGE_LAYOUT_UNDEFINED;
        ImageSpec.Samples = VK_SAMPLE_COUNT_1_BIT;
        ImageSpec.Usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        ImageSpec.SharingMode = VK_SHARING_MODE_EXCLUSIVE;
        ImageSpec.Tiling = VK_IMAGE_TILING_OPTIMAL;

        CreateVulkanImage(_Data->Allocator, _Data->DepthData.image, _Data->DepthData.allocation, ImageSpec);

        // Create image view
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = _Data->DepthData.image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = _Data->DepthData.format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        vkCreateImageView(_Data->Device.GetHandle(), &viewInfo, nullptr, &_Data->DepthData.imageview);

        // Transition to correct layout
        SingleTimeCommandBuffer CommandSpec{};
        CommandSpec.Family = QueueFamilies::GRAPHICS;
        _ResourceFactory->BeginSingleTimeCommand(CommandSpec);
        _TransitionAttachmentImageLayout(CommandSpec.Cmd, _Data->DepthData.image, _Data->DepthData.format,
                                         VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                         VK_IMAGE_LAYOUT_UNDEFINED);
        _ResourceFactory->EndSingleTimeCommand(CommandSpec);
    }

    VkFormat VulkanRenderApi::FindDepthFormat()
    {
        return FindSupportedFormat(
            {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    }

    VkFormat VulkanRenderApi::FindSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
    {
        for (VkFormat format : candidates)
        {
            VkFormatProperties props = VulkanUtils::GetPhysicalDeviceFormatProperties(_Data->PhysicalDevices[_Data->ActivePhysicalDeviceIndex], format);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
                return format;
            else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
                return format;
        }

        throw std::runtime_error("failed to find supported format!");
    }

    bool VulkanRenderApi::HasStencilComponent(VkFormat format)
    {
        return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
    }

    void VulkanRenderApi::_TransitionImageLayout(VkCommandBuffer commandBuffer, VkImage Image, VkImageLayout OldLayout, VkImageLayout NewLayout)
    {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = OldLayout;
        barrier.newLayout = NewLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = Image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (OldLayout == VK_IMAGE_LAYOUT_UNDEFINED)
        {
            if (NewLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
            {
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

                sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            }
        }
        else if (OldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && NewLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else
        {
            throw std::invalid_argument("unsupported layout transition!");
        }

        vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    }

    void VulkanRenderApi::_TransitionAttachmentImageLayout(VkCommandBuffer commandBuffer, VkImage Image, VkFormat Format, VkImageLayout NewLayout, VkImageLayout OldLayout)
    {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = OldLayout;
        barrier.newLayout = NewLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = Image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

        if (NewLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            if (HasStencilComponent(Format))
            {
                barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
            }
        }
        else
        {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        }

        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        VkPipelineStageFlags sourceStage, destinationStage;

        if (OldLayout == VK_IMAGE_LAYOUT_UNDEFINED && NewLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        }
        else if (OldLayout == VK_IMAGE_LAYOUT_UNDEFINED && NewLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        }
        else
        {
            throw std::invalid_argument("unsupported layout transition!");
        }

        vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    }

#pragma endregion
#pragma region VulkanUtils
    void VulkanUtils::GetInstanceExtensions(std::vector<VkExtensionProperties> &Props)
    {
        uint32_t extcount;
        vkEnumerateInstanceExtensionProperties(nullptr, &extcount, nullptr);

        Props.resize(extcount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extcount, Props.data());
    }

    void VulkanUtils::GetInstanceLayers(std::vector<VkLayerProperties> &Props)

    {
        uint32_t count;
        vkEnumerateInstanceLayerProperties(&count, nullptr);

        Props.resize(count);
        vkEnumerateInstanceLayerProperties(&count, Props.data());
    }

    bool VulkanUtils::CheckIfExtensionExist(std::vector<const char *> &Extensions)
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

    bool VulkanUtils::CheckIfLayerExist(std::vector<const char *> &Layers)
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

    VulkanVersion VulkanUtils::GetInstanceVulkanVersion()
    {
        uint32_t ApiVersion = 0;
        vkEnumerateInstanceVersion(&ApiVersion);

        VulkanVersion version;
        version.Max = VK_API_VERSION_MAJOR(ApiVersion);
        version.Min = VK_API_VERSION_MINOR(ApiVersion);
        version.Patch = VK_API_VERSION_PATCH(ApiVersion);
        return version;
    }

    VkFormatProperties VulkanUtils::GetPhysicalDeviceFormatProperties(VulkanPhysicalDevice &Device, VkFormat Format)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(Device.PhysicalDevice, Format, &props);
        return props;
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
