#pragma once

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

    class VulkanVertexBuffer
    {
    public:
        VulkanVertexBuffer() {}
        ~VulkanVertexBuffer() {}

        bool Init(VkDevice device, VkPhysicalDevice physicalDevice, void *data, int FloatCount);
        void Bind(VkDevice device);
        void Destroy(VkDevice device);

        VkBuffer GetHandle() { return _Buffer.GetHandle(); }

    private:
        VulkanBuffer _Buffer;
    };

    enum class VulkanIndexBufferType
    {
        UINT_8,
            UINT_16,
            UINT_32,
            UINT_64,
    };

    class VulkanIndexBuffer
    {
    public:
        VulkanIndexBuffer() {}
        ~VulkanIndexBuffer() {}

        bool Init(VkDevice device, VkPhysicalDevice physicalDevice, void *data, int Uintcount, VulkanIndexBufferType Type);
        void Bind(VkDevice device);
        void Destroy(VkDevice device);

        uint32_t Size() const { return _Size; }
        VkIndexType GetType();

        VkBuffer GetHandle() { return _Buffer.GetHandle(); }

    private:
        VulkanBuffer _Buffer;
        uint32_t _Size = 0;
        VulkanIndexBufferType _Type; 
    };
} // namespace VEngine
