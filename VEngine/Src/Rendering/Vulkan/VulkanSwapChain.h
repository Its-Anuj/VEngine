#pragma once

namespace VEngine
{
    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };
    
    class VulkanSwapChain
    {
    public:
        VulkanSwapChain() {}
        ~VulkanSwapChain() {}

        void Init(void *Surface);
        void Destroy();

        // Const getters
        VkFormat GetSwapChainImageFormat() const { return _SwapChainImageFormat; }
        VkExtent2D GetSwapChainExtent() const { return _SwapChainExtent; }
        const std::vector<VkImageView> &GetSwapChainImageViews() const { return _SwapChainImageViews; }

        VkSwapchainKHR GetHandle() const { return _SwapChain; }

    private:
        void _CreateSwapChain(void *Surface);
        void _CreateImageViews();

    private:
        VkFormat _SwapChainImageFormat;
        VkExtent2D _SwapChainExtent;
        std::vector<VkImageView> _SwapChainImageViews;

        std::vector<VkImage> _SwapChainImages;
        VkSwapchainKHR _SwapChain;
    };

    SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
} // namespace VEngine
