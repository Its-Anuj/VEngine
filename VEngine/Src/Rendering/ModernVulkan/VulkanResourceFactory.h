#pragma once

#include "Buffers.h"

namespace VEngine
{
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

} // namespace VEngien
