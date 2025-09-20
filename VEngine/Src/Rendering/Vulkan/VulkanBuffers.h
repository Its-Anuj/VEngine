#pragma once

#include "Buffers.h"

namespace VEngine
{
    struct VulkanBufferSpec
    {
        VkDevice Device;
        VkPhysicalDevice PhysicalDevice;
        void *Data;
        int SizeInBytes;
        VkBufferUsageFlags UsageType;
        VkSharingMode SharingMode;
        VkMemoryPropertyFlags UsingProperties;
    };

    class VulkanBuffer
    {
    public:
        VulkanBuffer() {}
        ~VulkanBuffer() {}

        bool Init(const VulkanBufferSpec &Spec);
        void Bind(VkDevice device);
        void Destroy(VkDevice device);
        void MapMemory(VkDevice device, int Offset, int Size, void *Data);

        VkBuffer GetHandle() { return _Buffer; }

    private:
        VkBuffer _Buffer;
        VkDeviceMemory _Memory;
    };

    class VulkanVertexBuffer : public VertexBuffer
    {
    public:
        VulkanVertexBuffer() {}
        ~VulkanVertexBuffer() {}

        bool Init(float *data, int FloatCount) override;
        void Bind() override;
        void Destroy() override;

        VkBuffer GetHandle() { return _Buffer.GetHandle(); }

    private:
        VulkanBuffer _Buffer;
    };

    class VulkanIndexBuffer : public IndexBuffer
    {
    public:
        VulkanIndexBuffer() {}
        ~VulkanIndexBuffer() {}

        bool Init(void *data, int Uintcount, IndexBufferType Type) override;
        void Bind() override;
        void Destroy() override;

        uint32_t Size() const  override{ return _Size; }
        VkIndexType GetType();

        VkBuffer GetHandle() { return _Buffer.GetHandle(); }

    private:
        VulkanBuffer _Buffer;
        uint32_t _Size = 0;
        IndexBufferType _Type;
    };
} // namespace VEngine
