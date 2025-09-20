#include "VeVPCH.h"
#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan.h"
#include "VulkanBuffers.h"
#include "VulkanDevice.h"
#include "VulkanRenderer.h"

namespace VEngine
{
    VkIndexType IndexBufferTypeToVk(IndexBufferType type)
    {
        switch (type)
        {
        case IndexBufferType::UINT_8 :
            return VK_INDEX_TYPE_UINT16;
        case IndexBufferType::UINT_16 :
            return VK_INDEX_TYPE_UINT16;
        case IndexBufferType::UINT_32 :
            return VK_INDEX_TYPE_UINT16;
        case IndexBufferType::UINT_64 :
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

    bool VulkanBuffer::Init(const VulkanBufferSpec &Spec)
    {
        _Buffer = CreateBuffer(Spec.Device, Spec.SizeInBytes, Spec.UsageType, Spec.SharingMode);

        _Memory = AllocateMemory(Spec.Device, Spec.PhysicalDevice, _Buffer, Spec.UsingProperties);

        MapMemory(Spec.Device, 0, Spec.SizeInBytes, Spec.Data);

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

    bool VulkanVertexBuffer::Init(float *VerticesData, int FloatCount)
    {
        auto device = GetRuntimeData()->ActiveDevice->GetHandle();
        VulkanBufferSpec Spec;
        Spec.Device = device;
        Spec.PhysicalDevice = GetRuntimeData()->ActivePhysicalDevice->GetHandle();
        Spec.Data = (void*)VerticesData;
        Spec.SharingMode = VK_SHARING_MODE_EXCLUSIVE;
        Spec.UsageType = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        Spec.UsingProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        Spec.SizeInBytes = sizeof(float) * FloatCount;

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

    bool VulkanIndexBuffer::Init(void *data, int Uintcount, IndexBufferType Type)
    {
        auto device = GetRuntimeData()->ActiveDevice->GetHandle();
        VulkanBufferSpec Spec;
        Spec.Device = device;
        Spec.PhysicalDevice = GetRuntimeData()->ActivePhysicalDevice->GetHandle();
        Spec.Data = data;
        Spec.SharingMode = VK_SHARING_MODE_EXCLUSIVE;
        Spec.UsageType = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        Spec.UsingProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        Spec.SizeInBytes = sizeof(unsigned int) * Uintcount;

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
} // namespace VEngine
