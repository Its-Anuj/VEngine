#pragma once

namespace VEngine
{
    struct VulkanRenderPassSpec
    {
    };

    struct RenderPassAttachmentSpec
    {
        
    };

    class VulkanRenderPassAttachment
    {
    public:
        VulkanRenderPassAttachment() {}
        ~VulkanRenderPassAttachment() {}

        void Fill(const RenderPassAttachmentSpec& Spec);

    private:
    };

    class VulkanRenderPass
    {
    public:
    private:
    };

} // namespace VEngine
