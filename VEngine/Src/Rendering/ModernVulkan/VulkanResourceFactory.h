#pragma once

#include "Buffers.h"

namespace VEngine
{
    struct SingleTimeCommandBuffer
    {
        VkCommandBuffer Cmd;
        QueueFamilies Family;
    };

    struct VulkanResourceFactory : public ResourceFactory
    {
    public:
        VulkanResourceFactory(VulkanRenderData *data) : _Data(data) {}
        ~VulkanResourceFactory();

        void Terminate();

        Ref<VertexBuffer> CreateVertexBuffer(const VertexBufferDesc &desc) override;
        bool DeleteVertexBuffer(const Ref<VertexBuffer> &VB) override;

        Ref<IndexBuffer> CreateIndexBuffer(const IndexBufferDesc &desc) override;
        bool DeleteIndexBuffer(const Ref<IndexBuffer> &IB) override;

        void CopyBuffer(VkBuffer Src, VkBuffer Dst, VkDeviceSize Size, SingleTimeCommandBuffer &Spec);

        void BeginSingleTimeCommand(SingleTimeCommandBuffer &Spec);
        void EndSingleTimeCommand(SingleTimeCommandBuffer &Spec);

    private:
        VulkanRenderData *_Data;
    };

    struct VulkanBuffer
    {
    };

    struct VulkanStageBufferSpec
    {
        uint32_t Size;
        void *Data;
    };

    struct VulkanStageBuffer
    {
    public:
        VulkanStageBuffer(VmaAllocator Allocator, const VulkanStageBufferSpec &spec);
        ~VulkanStageBuffer() {}

        void Destroy(VmaAllocator Allocator);

        VkBuffer GetHandle() { return _Buffer; }
        VmaAllocationInfo GetInfo() { return _AllocatoinInfo; }

        void CopyBuffer(VkCommandBuffer TransferCommandBuffer, VkBuffer DstBuffer, VkDeviceSize Size);

    private:
        VkBuffer _Buffer;
        VmaAllocation _Allocation;
        VmaAllocationInfo _AllocatoinInfo;
    };

    class VulkanVertexBuffer : public VertexBuffer
    {
    public:
        VulkanVertexBuffer(VmaAllocator Allocator, const VertexBufferDesc &desc);
        ~VulkanVertexBuffer() {}

        virtual void UploadData(const void *data, uint32_t size) override;
        void Destroy(VmaAllocator Allocator);

        VkBuffer GetHandle() { return _Buffer; }

    private:
        VkBuffer _Buffer;
        VmaAllocation _Allocation;
        VmaAllocationInfo _AllocatoinInfo;
    };

    struct UniformBufferDesc
    {
        size_t Size;
    };

    class VulkanUniformBuffer
    {
    public:
        VulkanUniformBuffer(VmaAllocator Allocator, const UniformBufferDesc &desc);
        ~VulkanUniformBuffer() {}

        void UploadData(const void *data, uint32_t size);
        void Destroy(VmaAllocator Allocator);

        VkBuffer GetHandle() { return _Buffer; }

    private:
        size_t _Size = 0;
        VkBuffer _Buffer;
        VmaAllocation _Allocation;
        VmaAllocationInfo _AllocatoinInfo;
    };

    class VulkanIndexBuffer : public IndexBuffer
    {
    public:
        VulkanIndexBuffer(VmaAllocator Allocator, const IndexBufferDesc &desc);
        ~VulkanIndexBuffer() {}

        void UploadData(const void *data, uint32_t size) override;
        void Destroy(VmaAllocator Allocator);

        VkBuffer GetHandle() { return _Buffer; }

    private:
        VkBuffer _Buffer;
        VmaAllocation _Allocation;
        VmaAllocationInfo _AllocatoinInfo;
    };

    enum class ShaderDataType
    {
        FLOAT1,
        FLOAT2,
        FLOAT3,
        FLOAT4,
        INT1,
        INT2,
        INT3,
        INT4,
        UINT1,
        UINT2,
        UINT3,
        UINT4,
    };

    struct VulkanVertexAttribute
    {
        int Binding = 0;
        int Location = 0;
        ShaderDataType Type;
        const char *Name;

        VulkanVertexAttribute(int pBinding, int pLocation, ShaderDataType pType, const char *pName)
            : Binding(pBinding), Location(pLocation), Type(pType), Name(pName)
        {
        }
    };

    struct VulkanGraphicsPipelineSpec
    {
        std::vector<const char *> Paths;
        std::vector<VulkanVertexAttribute> Attributes;
        VkDevice device;
        const char *Name;
        VkFormat SwapChainFormat;
        std::vector<VkDescriptorSetLayout> DescLayouts;
        bool UseDepth = false;
        VkFormat DepthFormat;
    };

    class VulkanGraphicsPipeline
    {
    public:
        VulkanGraphicsPipeline() {}
        ~VulkanGraphicsPipeline() {}

        void Init(const VulkanGraphicsPipelineSpec &Spec);
        void Destroy(VkDevice device);

        VkPipeline GetHandle() const { return _Pipeline; }
        VkPipelineLayout GetLayout() const { return _PipelineLayout; }

    private:
        VkPipeline _Pipeline;
        VkPipelineLayout _PipelineLayout;
    };

    struct VulkanTextures
    {
        VkImage image;
        VkImageView imageView;
        VkSampler sampler;
        VmaAllocation allocation;
        uint32_t width, height;
        VkFormat format;
    };

    struct VulkanTextureDescriptor
    {
        VkDescriptorSetLayout layout;
        VkDescriptorPool pool;
        std::vector<VkDescriptorSet> sets;
    };

} // namespace VEngien
