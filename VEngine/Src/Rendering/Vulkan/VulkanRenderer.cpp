#include <limits>
#include "VeVPCH.h"
#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan.h"
#include "VulkanDevice.h"
#include "VulkanBuffers.h"
#include "VulkanRenderer.h"
#include "VulkanGraphicsPipeline.h"
#include "VulkanSwapChain.h"
#define VMA_IMPLEMENTATION
// Optional: If you use a dynamic loader like Volk, you might need these:
// #define VMA_STATIC_VULKAN_FUNCTIONS 0
// #define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#include <vk_mem_alloc.h> // The include path is now provided by the CMake target

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>

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
    struct UniformBufferObject
    {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    };

    struct VulkanRendererData
    {
        VkInstance Instance;
        VkDebugUtilsMessengerEXT DebugMessenger;
        VkSurfaceKHR Surface;

        // Devices
        std::vector<VulkanPhysicalDevice> PhysicalDevices;
        std::vector<VulkanDevice> Devices;
        VulkanPhysicalDevice *ActivePhysicalDevice;
        VulkanDevice *ActiveDevice;
        int ActivePhysicalDeviceIndex;
        int ActiveDeviceIndex;

        VulkanSwapChain SwapChainKHR;
        std::vector<VkFramebuffer> SwapChainFrameBuffers;
        VkRenderPass RenderPass;

        VkCommandPool CommandPool;
        VkCommandPool TransferCommandPool;
        std::vector<VkCommandBuffer> CommandBuffers;

        VkPipelineLayout PipelineLayout;

        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;
        std::vector<VkFence> inFlightFences;

        VkCommandBuffer ActiveCommandBuffer;
        uint32_t CurrentFrame = 0;

        VkPipeline GraphicsPipeline;
        VulkanShader Shader;
        bool FrameBufferResized = false;
        struct
        {
            float x, y;
        } FrameBufferSize;
        uint32_t CurrentImageIndex = 0;

        VkDescriptorSetLayout DescriptorLayout;
        std::vector<VulkanUniformBuffer> UniformBuffers;
        std::vector<void *> UniformBuffersMapped;
        VkDescriptorPool DescriptorPool;
        std::vector<VkDescriptorSet> DescriptorSets;

        VmaAllocator Vmmm;
    };

    static VulkanRuntimeData *g_RuntimeData;

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

    void VulkanRenderer::Begin(const RenderPassSpec &Spec)
    {
        vkWaitForFences(_Data->ActiveDevice->GetHandle(), 1, &_Data->inFlightFences[_Data->CurrentFrame], VK_TRUE, UINT64_MAX);

        VkResult result = vkAcquireNextImageKHR(_Data->ActiveDevice->GetHandle(), _Data->SwapChainKHR.GetHandle(), UINT64_MAX, _Data->imageAvailableSemaphores[_Data->CurrentFrame], VK_NULL_HANDLE, &_Data->CurrentImageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            _ReCreateSwapChain();
            return;
        }
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        {
            throw std::runtime_error("failed to acquire swap chain image!");
        }
        // Only reset the fence if we are submitting work
        vkResetFences(_Data->ActiveDevice->GetHandle(), 1, &_Data->inFlightFences[_Data->CurrentFrame]);

        vkResetCommandBuffer(_Data->CommandBuffers[_Data->CurrentFrame], 0);

        auto commandBuffer = _Data->CommandBuffers[_Data->CurrentFrame];
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;                  // Optional
        beginInfo.pInheritanceInfo = nullptr; // Optional

        _Data->ActiveCommandBuffer = nullptr;

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
            throw std::runtime_error("failed to begin recording command buffer!");
        _Data->ActiveCommandBuffer = _Data->CommandBuffers[_Data->CurrentFrame];

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = _Data->RenderPass;
        renderPassInfo.framebuffer = _Data->SwapChainFrameBuffers[_Data->CurrentImageIndex];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = _Data->SwapChainKHR.GetSwapChainExtent();

        VkClearValue clearColor = {{{Spec.ClearColor.x, Spec.ClearColor.y, Spec.ClearColor.z, Spec.ClearColor.w}}};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(_Data->SwapChainKHR.GetSwapChainExtent().width);
        viewport.height = static_cast<float>(_Data->SwapChainKHR.GetSwapChainExtent().height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = _Data->SwapChainKHR.GetSwapChainExtent();
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    }

    void VulkanRenderer::End()
    {
        auto commandBuffer = _Data->CommandBuffers[_Data->CurrentFrame];
        vkCmdEndRenderPass(commandBuffer);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
            throw std::runtime_error("failed to record command buffer!");
    }

    void VulkanRenderer::Submit(std::shared_ptr<VertexBuffer> &vb, std::shared_ptr<IndexBuffer> &ib)
    {
        auto VulkanVB = std::static_pointer_cast<VulkanVertexBuffer>(vb);
        auto VulkanIB = std::static_pointer_cast<VulkanIndexBuffer>(ib);

        auto commandBuffer = _Data->CommandBuffers[_Data->CurrentFrame];
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _Data->GraphicsPipeline);

        VkBuffer vertexBuffers[] = {VulkanVB->GetHandle()};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, VulkanIB->GetHandle(), 0, VulkanIB->GetType());

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _Data->PipelineLayout, 0, 1, &_Data->DescriptorSets[_Data->CurrentFrame], 0, nullptr);

        vkCmdDrawIndexed(commandBuffer, VulkanIB->Size(), 1, 0, 0, 0);
    }

    void VulkanRenderer::_CreateSyncObject()
    {
        _Data->inFlightFences.resize(_Spec.FramesInFlightCount);
        _Data->imageAvailableSemaphores.resize(_Spec.FramesInFlightCount);
        _Data->renderFinishedSemaphores.resize(_Spec.FramesInFlightCount);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < _Spec.FramesInFlightCount; i++)
        {
            if (vkCreateSemaphore(_Data->ActiveDevice->GetHandle(), &semaphoreInfo, nullptr, &_Data->imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(_Data->ActiveDevice->GetHandle(), &semaphoreInfo, nullptr, &_Data->renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(_Data->ActiveDevice->GetHandle(), &fenceInfo, nullptr, &_Data->inFlightFences[i]) != VK_SUCCESS)
                throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }

    void VulkanRenderer::_UpdateUniformBuffer(uint32_t currentImage)
    {
        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        UniformBufferObject ubo{};
        ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.proj = glm::perspective(glm::radians(45.0f), _Data->SwapChainKHR.GetSwapChainExtent().width / (float)_Data->SwapChainKHR.GetSwapChainExtent().height, 0.1f, 10.0f);
        ubo.proj[1][1] *= -1;

        memcpy(_Data->UniformBuffers[currentImage].GetMappedPointer(), &ubo, sizeof(ubo));
    }

    void VulkanRenderer::_CleanUpSwapChain()
    {
        for (auto framebuffer : _Data->SwapChainFrameBuffers)
            vkDestroyFramebuffer(_Data->ActiveDevice->GetHandle(), framebuffer, nullptr);
        _Data->SwapChainKHR.Destroy();

        PRINTLN("[VULKAN]: SwapChain Cleaned!")
    }

    void VulkanRenderer::_ReCreateSwapChain()
    {
        vkDeviceWaitIdle(_Data->ActiveDevice->GetHandle());
        _CleanUpSwapChain();

        _CreateSwapChain();
        _CreateFrameBuffers();
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
        for (size_t i = 0; i < _Spec.FramesInFlightCount; i++)
        {
            vkDestroySemaphore(_Data->ActiveDevice->GetHandle(), _Data->renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(_Data->ActiveDevice->GetHandle(), _Data->imageAvailableSemaphores[i], nullptr);
            vkDestroyFence(_Data->ActiveDevice->GetHandle(), _Data->inFlightFences[i], nullptr);
        }

        for (int i = 0; i < _Spec.FramesInFlightCount; i++)
            _Data->UniformBuffers[i].Destroy();

        vkDestroyCommandPool(_Data->ActiveDevice->GetHandle(), _Data->TransferCommandPool, nullptr);
        vkDestroyCommandPool(_Data->ActiveDevice->GetHandle(), _Data->CommandPool, nullptr);
        for (auto framebuffer : _Data->SwapChainFrameBuffers)
            vkDestroyFramebuffer(_Data->ActiveDevice->GetHandle(), framebuffer, nullptr);

        vkDestroyDescriptorPool(_Data->ActiveDevice->GetHandle(), _Data->DescriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(_Data->ActiveDevice->GetHandle(), _Data->DescriptorLayout, nullptr);

        vkDestroyPipeline(_Data->ActiveDevice->GetHandle(), _Data->GraphicsPipeline, nullptr);
        vkDestroyPipelineLayout(_Data->ActiveDevice->GetHandle(), _Data->PipelineLayout, nullptr);
        vkDestroyRenderPass(_Data->ActiveDevice->GetHandle(), _Data->RenderPass, nullptr);

        _Data->SwapChainKHR.Destroy();

        for (auto Device : _Data->Devices)
            Device.Destroy();

        if (_Spec.EnableValidationLayer)
            DestroyDebugUtilsMessengerEXT(_Data->Instance, _Data->DebugMessenger, nullptr);

        vkDestroySurfaceKHR(_Data->Instance, _Data->Surface, nullptr);

        vkDestroyInstance(_Data->Instance, nullptr);

        delete g_RuntimeData;
        g_RuntimeData = nullptr;
        PRINTLN("Terminated vulkan!")
    }

    void VulkanRenderer::Present()
    {
        VkSemaphore signalSemaphores[] = {_Data->renderFinishedSemaphores[_Data->CurrentFrame]};

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = {_Data->SwapChainKHR.GetHandle()};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &_Data->CurrentImageIndex;
        presentInfo.pResults = nullptr; // Optional

        auto result = vkQueuePresentKHR(_Data->ActiveDevice->GetQueues(QueueFamilies::PRESENT), &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || _Data->FrameBufferResized)
        {
            _Data->FrameBufferResized = false;
            _ReCreateSwapChain();
        }
        else if (result != VK_SUCCESS)
            throw std::runtime_error("failed to present swap chain image!");

        _Data->CurrentFrame = (_Data->CurrentFrame + 1) % _Spec.FramesInFlightCount;
    }

    void VulkanRenderer::Finish()
    {
        vkDeviceWaitIdle(_Data->ActiveDevice->GetHandle());
    }

    void VulkanRenderer::CopyBuffer(VulkanBuffer &SrcBuffer, VulkanBuffer &DstBuffer, uint32_t Size)
    {
        VkCommandBuffer commandbuffer;

        VkCommandBufferAllocateInfo allocinfo{};
        allocinfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocinfo.commandBufferCount = 1;
        allocinfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocinfo.commandPool = _Data->TransferCommandPool;

        if (vkAllocateCommandBuffers(_Data->ActiveDevice->GetHandle(), &allocinfo, &commandbuffer) != VK_SUCCESS)
            throw std::runtime_error("Bad");

        VkCommandBufferBeginInfo begininfo{};
        begininfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begininfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandbuffer, &begininfo);

        VkBufferCopy buffercopy{};
        buffercopy.size = Size;
        buffercopy.dstOffset = 0;
        buffercopy.srcOffset = 0;
        vkCmdCopyBuffer(commandbuffer, SrcBuffer.GetHandle(), DstBuffer.GetHandle(), 1, &buffercopy);

        vkEndCommandBuffer(commandbuffer);

        VkSubmitInfo submitinfo{};
        submitinfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitinfo.pCommandBuffers = &commandbuffer;
        submitinfo.commandBufferCount = 1;

        vkQueueSubmit(_Data->ActiveDevice->GetQueues(QueueFamilies::TRANSFER), 1, &submitinfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(_Data->ActiveDevice->GetQueues(QueueFamilies::TRANSFER));

        vkFreeCommandBuffers(_Data->ActiveDevice->GetHandle(), _Data->TransferCommandPool, 1, &commandbuffer);
    }

    void VulkanRenderer::Render()
    {
        _UpdateUniformBuffer(_Data->CurrentFrame);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = {_Data->imageAvailableSemaphores[_Data->CurrentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &_Data->CommandBuffers[_Data->CurrentFrame];

        VkSemaphore signalSemaphores[] = {_Data->renderFinishedSemaphores[_Data->CurrentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(_Data->ActiveDevice->GetQueues(QueueFamilies::GRAPHICS), 1, &submitInfo, _Data->inFlightFences[_Data->CurrentFrame]) != VK_SUCCESS)
            throw std::runtime_error("failed to submit draw command buffer!");
    }

    void VulkanRenderer::FrameBufferResize(int x, int y)
    {
        _Data->FrameBufferResized = true;
        _Data->FrameBufferSize = {(float)x, (float)y};
    }

    void VulkanRenderer::Init(void *Spec)
    {
        _Data = new VulkanRendererData();
        _Spec = *(VulkanRenderSpec *)Spec;
        _Data->FrameBufferSize.x = _Spec.FrameBufferWidth;
        _Data->FrameBufferSize.y = _Spec.FrameBufferHeight;

        // For Swapchain
        _Spec.DeviceRequirerdExtensions.push_back("VK_KHR_swapchain");
        g_RuntimeData = new VulkanRuntimeData();

        _CreateInstance();
        _CreateDebugMessenger();
        _CreateWindowSurface();
        _CreateSuitablePhysicalDevice();
        _CreateSuitableLogicalDevice();

        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.physicalDevice = _Data->ActivePhysicalDevice->GetHandle();
        allocatorInfo.device = _Data->ActiveDevice->GetHandle();
        allocatorInfo.instance = _Data->Instance;

        vmaCreateAllocator(&allocatorInfo, &_Data->Vmmm);

        _CreateSwapChain();
        _CreateRenderPass();
        _CreateDescriptorSetLayout();
        _CreateGraphicsPipeline();
        _CreateUniformBuffers();
        _CreateDescriptorPool();
        _CreateDescriptorSets();
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
        g_RuntimeData->ActivePhysicalDevice = _Data->ActivePhysicalDevice;
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
        g_RuntimeData->ActiveDevice = _Data->ActiveDevice;
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
        g_RuntimeData->Surface = &_Data->Surface;
    }

    void VulkanRenderer::_CreateSwapChain()
    {
        _Data->SwapChainKHR.Init((void *)_Data->Surface);
        g_RuntimeData->SwapChain = &_Data->SwapChainKHR;
    }

    void VulkanRenderer::_CreateDescriptorSetLayout()
    {
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &uboLayoutBinding;

        if (vkCreateDescriptorSetLayout(_Data->ActiveDevice->GetHandle(), &layoutInfo, nullptr, &_Data->DescriptorLayout) != VK_SUCCESS)
            throw std::runtime_error("failed to create descriptor set layout!");
    }

    void VulkanRenderer::_CreateDescriptorSets()
    {
        std::vector<VkDescriptorSetLayout> layouts(_Spec.FrameBufferHeight, _Data->DescriptorLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = _Data->DescriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(_Spec.FramesInFlightCount);
        allocInfo.pSetLayouts = layouts.data();

        _Data->DescriptorSets.resize(_Spec.FramesInFlightCount);
        if (vkAllocateDescriptorSets(_Data->ActiveDevice->GetHandle(), &allocInfo, _Data->DescriptorSets.data()) != VK_SUCCESS)
            throw std::runtime_error("failed to allocate descriptor sets!");

        for (size_t i = 0; i < _Spec.FramesInFlightCount; i++)
        {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = _Data->UniformBuffers[i].GetHandle();
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            VkWriteDescriptorSet descriptorWrite{};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = _Data->DescriptorSets[i];
            descriptorWrite.dstBinding = 0;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.pBufferInfo = &bufferInfo;
            descriptorWrite.pImageInfo = nullptr;       // Optional
            descriptorWrite.pTexelBufferView = nullptr; // Optional

            vkUpdateDescriptorSets(_Data->ActiveDevice->GetHandle(), 1, &descriptorWrite, 0, nullptr);
        }
    }

    void VulkanRenderer::_CreateDescriptorPool()
    {
        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize.descriptorCount = static_cast<uint32_t>(_Spec.FramesInFlightCount);

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;
        poolInfo.maxSets = static_cast<uint32_t>(_Spec.FramesInFlightCount);

        if (vkCreateDescriptorPool(_Data->ActiveDevice->GetHandle(), &poolInfo, nullptr, &_Data->DescriptorPool) != VK_SUCCESS)
            throw std::runtime_error("failed to create descriptor pool!");
    }

    void VulkanRenderer::_CreateGraphicsPipeline()
    {
        ShaderSpec ShaderSpec;
        ShaderSpec.Name = "main";
        ShaderSpec.Paths.push_back("vert.spv");
        ShaderSpec.Paths.push_back("frag.spv");
        ShaderSpec.UsingTypes.push_back(ShaderType::SHDAER_TYPE_VERTEX);
        ShaderSpec.UsingTypes.push_back(ShaderType::SHDAER_TYPE_FRAGMENT);
        ShaderSpec.Attribs.push_back({"Position", ShaderVertexTypes::FLOAT2, 0, 0});
        ShaderSpec.Attribs.push_back({"Color", ShaderVertexTypes::FLOAT3, 0, 1});
        _Data->Shader.Init(ShaderSpec);

        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR};

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        std::vector<VkVertexInputAttributeDescription> attribdes;
        VkVertexInputBindingDescription bindingDes;

        VkPipelineVertexInputStateCreateInfo vertexInputInfo = _Data->Shader.GetInfo(attribdes, bindingDes);

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)_Data->SwapChainKHR.GetSwapChainExtent().width;
        viewport.height = (float)_Data->SwapChainKHR.GetSwapChainExtent().height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = _Data->SwapChainKHR.GetSwapChainExtent();

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

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &_Data->DescriptorLayout;

        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.pushConstantRangeCount = 0;    // Optional
        pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

        if (vkCreatePipelineLayout(_Data->ActiveDevice->GetHandle(), &pipelineLayoutInfo, nullptr, &_Data->PipelineLayout) != VK_SUCCESS)
            throw std::runtime_error("failed to create pipeline layout!");

        auto shaderStages = _Data->Shader.GetShaderStages();
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = shaderStages.size();
        pipelineInfo.pStages = shaderStages.data();
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

        _Data->Shader.Destroy();
    }
#pragma region CommandBuffers
    void VulkanRenderer::_CreateFrameBuffers()
    {
        _Data->SwapChainFrameBuffers.resize(_Data->SwapChainKHR.GetSwapChainImageViews().size());
        for (size_t i = 0; i < _Data->SwapChainKHR.GetSwapChainImageViews().size(); i++)
        {
            VkImageView attachments[] = {
                _Data->SwapChainKHR.GetSwapChainImageViews()[i]};

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = _Data->RenderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = _Data->SwapChainKHR.GetSwapChainExtent().width;
            framebufferInfo.height = _Data->SwapChainKHR.GetSwapChainExtent().height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(_Data->ActiveDevice->GetHandle(), &framebufferInfo, nullptr, &_Data->SwapChainFrameBuffers[i]) != VK_SUCCESS)
                throw std::runtime_error("failed to create framebuffer!");
        }
    }

    void VulkanRenderer::_CreateUniformBuffers()
    {
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);
        _Data->UniformBuffers.resize(_Spec.FramesInFlightCount);
        _Data->UniformBuffersMapped.resize(_Spec.FramesInFlightCount);

        for (int i = 0; i < _Spec.FramesInFlightCount; i++)
        {
            _Data->UniformBuffers[i].Init(_Data->UniformBuffersMapped[i], bufferSize, BufferTypes::DYNAMIC);
        }
    }

    void VulkanRenderer::_CreateCommandPool()
    {
        QueueFamilyIndices queueFamilyIndices = _Data->ActivePhysicalDevice->GetQueueFamilyIndicies();
        {
            VkCommandPoolCreateInfo poolInfo{};
            poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            poolInfo.queueFamilyIndex = queueFamilyIndices.Queues[QueueFamilies::GRAPHICS].value();

            if (vkCreateCommandPool(_Data->ActiveDevice->GetHandle(), &poolInfo, nullptr, &_Data->CommandPool) != VK_SUCCESS)
                throw std::runtime_error("failed to create command pool!");
        }
        {
            VkCommandPoolCreateInfo poolInfo{};
            poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            poolInfo.queueFamilyIndex = queueFamilyIndices.Queues[QueueFamilies::TRANSFER].value();

            if (vkCreateCommandPool(_Data->ActiveDevice->GetHandle(), &poolInfo, nullptr, &_Data->TransferCommandPool) != VK_SUCCESS)
                throw std::runtime_error("failed to create command pool!");
        }
    }

    void VulkanRenderer::_CreateCommandBuffer()
    {
        _Data->CommandBuffers.resize(_Spec.FramesInFlightCount);
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = _Data->CommandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t)_Data->CommandBuffers.size();

        if (vkAllocateCommandBuffers(_Data->ActiveDevice->GetHandle(), &allocInfo, _Data->CommandBuffers.data()) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }
#pragma endregion

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

            if (Indicies.Queues[QueueFamilies::TRANSFER].has_value())
                Scores[i] += 10000;

            if (!Device.GetFeatures().geometryShader)
                Scores[i] = 0;

            if (!Device.CheckExtensionsSupported(_Spec.DeviceRequirerdExtensions))
                Scores[i] = 0;
            else
            {
                bool swapChainAdequate = false;

                SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(Device.GetHandle(), _Data->Surface);
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
        colorAttachment.format = _Data->SwapChainKHR.GetSwapChainImageFormat();
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
#pragma region DebugMessenger
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
#pragma endregion

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

    VulkanRuntimeData *GetRuntimeData()
    {
        return g_RuntimeData;
    }
#pragma region SwapChain

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

    void VulkanSwapChain::Init(void *Surface)
    {
        _CreateSwapChain(Surface);
        _CreateImageViews();
    }

    void VulkanSwapChain::Destroy()
    {
        auto device = GetRuntimeData()->ActiveDevice->GetHandle();
        for (auto imageView : _SwapChainImageViews)
            vkDestroyImageView(device, imageView, nullptr);

        vkDestroySwapchainKHR(device, _SwapChain, nullptr);
    }

    void VulkanSwapChain::_CreateSwapChain(void *Surface)
    {
        auto Data = GetRuntimeData();
        SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(Data->ActivePhysicalDevice->GetHandle(), *Data->Surface);

        VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities, Data->FrameBufferSize.x, Data->FrameBufferSize.y);

        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
            imageCount = swapChainSupport.capabilities.maxImageCount;

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = *Data->Surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        QueueFamilyIndices indices = Data->ActivePhysicalDevice->GetQueueFamilyIndicies();
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

        if (vkCreateSwapchainKHR(Data->ActiveDevice->GetHandle(), &createInfo, nullptr, &_SwapChain) != VK_SUCCESS)
            throw std::runtime_error("failed to create swap chain!");

        vkGetSwapchainImagesKHR(Data->ActiveDevice->GetHandle(), _SwapChain, &imageCount, nullptr);
        _SwapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(Data->ActiveDevice->GetHandle(), _SwapChain, &imageCount, _SwapChainImages.data());

        _SwapChainImageFormat = surfaceFormat.format;
        _SwapChainExtent = extent;

        PRINTLN("[VULKAN]: SwapChain Created")
    }

    void VulkanSwapChain::_CreateImageViews()
    {
        auto device = GetRuntimeData()->ActiveDevice->GetHandle();
        _SwapChainImageViews.resize(_SwapChainImages.size());

        for (int i = 0; i < _SwapChainImages.size(); i++)
        {
            VkImageViewCreateInfo createinfo{};
            createinfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createinfo.image = _SwapChainImages[i];
            createinfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createinfo.format = _SwapChainImageFormat;

            createinfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createinfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createinfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createinfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

            createinfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createinfo.subresourceRange.baseMipLevel = 0;
            createinfo.subresourceRange.levelCount = 1;
            createinfo.subresourceRange.baseArrayLayer = 0;
            createinfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(device, &createinfo, nullptr, &_SwapChainImageViews[i]) != VK_SUCCESS)
                throw std::runtime_error("failed to create image views!");
        }
        PRINTLN("[VULKAN]: ImageView Created!")
    }
#pragma endregion

#pragma region Buffers
    VkIndexType IndexBufferTypeToVk(IndexBufferType type)
    {
        switch (type)
        {
        case IndexBufferType::UINT_8:
            return VK_INDEX_TYPE_UINT16;
        case IndexBufferType::UINT_16:
            return VK_INDEX_TYPE_UINT16;
        case IndexBufferType::UINT_32:
            return VK_INDEX_TYPE_UINT16;
        case IndexBufferType::UINT_64:
            return VK_INDEX_TYPE_UINT16;
        default:
            throw std::runtime_error("Format not supported!");
        }
    }

    uint32_t FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
                return i;

        throw std::runtime_error("failed to find suitable memory type!");
    }

    VkBuffer CreateBuffer(VkDevice device, int SizeInBytes, VkBufferUsageFlags UsageType, VkSharingMode SharingMode)
    {
        VkBuffer Buffer;

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = SizeInBytes;
        bufferInfo.usage = UsageType;
        bufferInfo.sharingMode = SharingMode;

        if (vkCreateBuffer(device, &bufferInfo, nullptr, &Buffer) != VK_SUCCESS)
            throw std::runtime_error("failed to create vertex buffer!");
        return Buffer;
    }

    VkDeviceMemory AllocateMemory(VkDevice device, VkPhysicalDevice physicalDevice, VkBuffer buffer, VkMemoryPropertyFlags properties)
    {
        VkDeviceMemory Memory;

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &Memory) != VK_SUCCESS)
            throw std::runtime_error("failed to allocate vertex buffer memory!");

        return Memory;
    }

    bool VulkanBuffer::Init(VulkanBufferSpec &Spec)
    {
        _Buffer = CreateBuffer(Spec.Device, Spec.SizeInBytes, Spec.UsageType, Spec.SharingMode);
        _Memory = AllocateMemory(Spec.Device, Spec.PhysicalDevice, _Buffer, Spec.MemoryPropsFlags);

        Bind(Spec.Device);

        if (Spec.Type == BufferTypes::DYNAMIC)
            _ForDynamicInit(Spec);
        else if (Spec.Type == BufferTypes::STATIC)
            _ForStaticInit(Spec);

        return true;
    }

    void VulkanBuffer::Bind(VkDevice device)
    {
        vkBindBufferMemory(device, _Buffer, _Memory, 0);
    }

    void VulkanBuffer::Destroy(VkDevice device)
    {
        vkDestroyBuffer(device, _Buffer, nullptr);
        vkFreeMemory(device, _Memory, nullptr);
    }

    void VulkanBuffer::MapMemory(VkDevice device, int Offset, int Size, void *Data)
    {
        Bind(device);

        void *data;
        vkMapMemory(device, _Memory, 0, Size, 0, &data);
        memcpy(data, Data, (size_t)Size);
        vkUnmapMemory(device, _Memory);
    }

    void VulkanBuffer::_CopyBuffer()
    {
    }

    void VulkanBuffer::_ForStaticInit(VulkanBufferSpec &Spec)
    {
        // Use stagin buffer
        // _Memory = AllocateMemory(Spec.Device, Spec.PhysicalDevice, _Buffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        void *data;

        vkMapMemory(Spec.Device, _Memory, 0, Spec.SizeInBytes, 0, &data);
        memcpy(data, Spec.Data, (size_t)Spec.SizeInBytes);
        vkUnmapMemory(Spec.Device, _Memory);
    }

    void VulkanBuffer::_ForDynamicInit(VulkanBufferSpec &Spec)
    {
        // For dynamic buffers, map and keep persistent mapping
        // _Memory = AllocateMemory(Spec.Device, Spec.PhysicalDevice, _Buffer, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        void *Data = nullptr;
        vkMapMemory(Spec.Device, _Memory, 0, Spec.SizeInBytes, 0, &Data);
        Spec.Data = Data;
    }
#pragma endregion
#pragma region VertexBuffer

    bool VulkanVertexBuffer::Init(float *VerticesData, int FloatCount, BufferTypes Type)
    {
        auto device = GetRuntimeData()->ActiveDevice->GetHandle();
        VulkanBufferSpec Spec;
        Spec.Device = device;
        Spec.PhysicalDevice = GetRuntimeData()->ActivePhysicalDevice->GetHandle();
        Spec.Data = (void *)VerticesData;
        Spec.SharingMode = VK_SHARING_MODE_EXCLUSIVE;
        Spec.UsageType = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        Spec.SizeInBytes = sizeof(float) * FloatCount;
        Spec.Type = Type;
        Spec.MemoryPropsFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

        _Buffer.Init(Spec);

        return true;
    }

    void VulkanVertexBuffer::Bind()
    {
        auto device = GetRuntimeData()->ActiveDevice->GetHandle();
        _Buffer.Bind(device);
    }

    void VulkanVertexBuffer::Destroy()
    {
        auto device = GetRuntimeData()->ActiveDevice->GetHandle();
        _Buffer.Destroy(device);
    }
#pragma endregion
#pragma region IndexBuffer
    bool VulkanIndexBuffer::Init(void *data, int Uintcount, IndexBufferType Type, BufferTypes BType)
    {
        auto device = GetRuntimeData()->ActiveDevice->GetHandle();
        VulkanBufferSpec Spec;
        Spec.Device = device;
        Spec.PhysicalDevice = GetRuntimeData()->ActivePhysicalDevice->GetHandle();
        Spec.Data = data;
        Spec.SharingMode = VK_SHARING_MODE_EXCLUSIVE;
        Spec.UsageType = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        Spec.SizeInBytes = sizeof(unsigned int) * Uintcount;
        Spec.MemoryPropsFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        Spec.Type = BType;

        _Buffer.Init(Spec);
        _Size = static_cast<uint32_t>(Uintcount);
        _Type = Type;

        return true;
    }

    void VulkanIndexBuffer::Bind()
    {
        auto device = GetRuntimeData()->ActiveDevice->GetHandle();
        _Buffer.Bind(device);
    }

    void VulkanIndexBuffer::Destroy()
    {
        auto device = GetRuntimeData()->ActiveDevice->GetHandle();
        _Buffer.Destroy(device);
    }

    VkIndexType VulkanIndexBuffer::GetType()
    {
        return IndexBufferTypeToVk(_Type);
    }
#pragma endregion

#pragma region UniformBuffers
    bool VulkanUniformBuffer::Init(void *data, int SizeinBytes, BufferTypes Type)
    {
        auto device = GetRuntimeData()->ActiveDevice->GetHandle();
        VulkanBufferSpec Spec;
        Spec.Device = device;
        Spec.PhysicalDevice = GetRuntimeData()->ActivePhysicalDevice->GetHandle();
        Spec.Data = data;
        Spec.SizeInBytes = SizeinBytes;
        Spec.UsageType = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        Spec.MemoryPropsFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

        Spec.SharingMode = VK_SHARING_MODE_EXCLUSIVE;
        Spec.Type = Type;

        _Buffer.Init(Spec);
        _MappedData = Spec.Data;

        return true;
    }

    void VulkanUniformBuffer::Destroy()
    {
        _Buffer.Destroy(GetRuntimeData()->ActiveDevice->GetHandle());
    }
#pragma endregion

#pragma region PhysicalDevice
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
            else if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT)
                Indicies.Queues[QueueFamilies::TRANSFER] = i;
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
#pragma endregion

#pragma region LogicalDevice
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

        if (indices.Queues[QueueFamilies::TRANSFER].has_value())
            vkGetDeviceQueue(_Device, indices.Queues[QueueFamilies::TRANSFER].value(), 0, &_Queues[QueueFamilies::TRANSFER]);

        return true;
    }

    void VulkanDevice::Destroy()
    {
        vkDestroyDevice(_Device, nullptr);
        _Device = nullptr;
        _Spec.PhysicalDevice = nullptr;
        _Spec.RequiredExtensions.clear();
    }
#pragma endregion
}

} // namespace VEngine
