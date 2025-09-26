#include "VeVPCH.h"

#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan/vulkan.h"

#include "vk_mem_alloc.h"
#include "VulkanRenderApi.h"
#include "VulkanDevice.h"
#include "VulkanResourceFactory.h"
#include "VulkanRenderData.h"

namespace VEngine
{
    VulkanResourceFactory::~VulkanResourceFactory()
    {
    }

    void VulkanResourceFactory::Terminate()
    {
        _Data = nullptr;
    }

    Ref<VertexBuffer> VulkanResourceFactory::CreateVertexBuffer(const VertexBufferDesc &desc)
    {
        Ref<VulkanVertexBuffer> VB = std::make_shared<VulkanVertexBuffer>(_Data->Allocator, desc);

        if (desc.Type == BufferTypes::STATIC_DRAW)
        {
            // Use staging buffer
            VulkanStageBufferSpec StagingBufferSpec{};
            StagingBufferSpec.Data = desc.Data;
            StagingBufferSpec.Size = desc.SizeInBytes;

            VulkanStageBuffer StagingBuffer(_Data->Allocator, StagingBufferSpec);

            SingleTimeCommandBuffer SingleComamndSpec{};
            SingleComamndSpec.Family = QueueFamilies::TRANSFER;

            this->BeginSingleTimeCommand(SingleComamndSpec);
            this->CopyBuffer(StagingBuffer.GetHandle(), VB->GetHandle(), desc.SizeInBytes, SingleComamndSpec);
            this->EndSingleTimeCommand(SingleComamndSpec);

            StagingBuffer.Destroy(_Data->Allocator);
        }

        return VB;
    }

    bool VulkanResourceFactory::DeleteVertexBuffer(const Ref<VertexBuffer> &VB)
    {
        auto VulkanVB = std::static_pointer_cast<VulkanVertexBuffer>(VB);
        VulkanVB->Destroy(_Data->Allocator);
        return true;
    }

    Ref<IndexBuffer> VulkanResourceFactory::CreateIndexBuffer(const IndexBufferDesc &desc)
    {
        Ref<VulkanIndexBuffer> IB = std::make_shared<VulkanIndexBuffer>(_Data->Allocator, desc);

        if (desc.Type == BufferTypes::STATIC_DRAW)
        {
            // Use staging buffer
            VulkanStageBufferSpec StagingBufferSpec{};
            StagingBufferSpec.Data = desc.Data;
            StagingBufferSpec.Size = desc.SizeInBytes;

            VulkanStageBuffer StagingBuffer(_Data->Allocator, StagingBufferSpec);

            SingleTimeCommandBuffer SingleComamndSpec{};
            SingleComamndSpec.Family = QueueFamilies::TRANSFER;

            this->BeginSingleTimeCommand(SingleComamndSpec);
            this->CopyBuffer(StagingBuffer.GetHandle(), IB->GetHandle(), desc.SizeInBytes, SingleComamndSpec);
            this->EndSingleTimeCommand(SingleComamndSpec);

            StagingBuffer.Destroy(_Data->Allocator);
        }
        
        return IB;
    }

    bool VulkanResourceFactory::DeleteIndexBuffer(const Ref<IndexBuffer> &IB)
    {
        auto VulkanIB = std::static_pointer_cast<VulkanIndexBuffer>(IB);
        VulkanIB->Destroy(_Data->Allocator);
        return true;
    }

    void VulkanResourceFactory::CopyBuffer(VkBuffer Src, VkBuffer Dst, VkDeviceSize Size, SingleTimeCommandBuffer &Spec)
    {
        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = 0;
        copyRegion.size = Size;
        vkCmdCopyBuffer(Spec.Cmd, Src, Dst, 1, &copyRegion);
    }

    void VulkanResourceFactory::BeginSingleTimeCommand(SingleTimeCommandBuffer &Spec)
    {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        if (Spec.Family == QueueFamilies::GRAPHICS)
            allocInfo.commandPool = _Data->GraphicsCommandPool;
        else if (Spec.Family == QueueFamilies::TRANSFER)
            allocInfo.commandPool = _Data->TransferCommandPool;

        vkAllocateCommandBuffers(_Data->Device.GetHandle(), &allocInfo, &Spec.Cmd);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(Spec.Cmd, &beginInfo);
    }

    void VulkanResourceFactory::EndSingleTimeCommand(SingleTimeCommandBuffer &Spec)
    {
        vkEndCommandBuffer(Spec.Cmd);

        VkSubmitInfo submit{};
        submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &Spec.Cmd;

        vkQueueSubmit(_Data->Device.GetQueue(Spec.Family), 1, &submit, VK_NULL_HANDLE);
        vkQueueWaitIdle(_Data->Device.GetQueue(Spec.Family));

        if (Spec.Family == QueueFamilies::GRAPHICS)
            vkFreeCommandBuffers(_Data->Device.GetHandle(), _Data->GraphicsCommandPool, 1, &Spec.Cmd);
        else if (Spec.Family == QueueFamilies::TRANSFER)
            vkFreeCommandBuffers(_Data->Device.GetHandle(), _Data->TransferCommandPool, 1, &Spec.Cmd);
    }

    // TODO: Make as muhc as posible few staging buffer
    VulkanStageBuffer::VulkanStageBuffer(VmaAllocator Allocator, const VulkanStageBufferSpec &spec)
    {
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = spec.Size;
        bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
        allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

        vmaCreateBuffer(Allocator, &bufferInfo, &allocInfo, &_Buffer, &_Allocation, &_AllocatoinInfo);

        // Copy pixel data to staging buffer
        void *data;
        vmaMapMemory(Allocator, _Allocation, &data);
        memcpy(data, spec.Data, static_cast<size_t>(spec.Size));
        vmaUnmapMemory(Allocator, _Allocation);
    }

    void VulkanStageBuffer::Destroy(VmaAllocator Allocator)
    {
        vmaDestroyBuffer(Allocator, _Buffer, _Allocation);
    }

    void VulkanStageBuffer::CopyBuffer(VkCommandBuffer TransferCommandBuffer, VkBuffer DstBuffer, VkDeviceSize Size)
    {
        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = 0;
        copyRegion.size = Size;
        vkCmdCopyBuffer(TransferCommandBuffer, _Buffer, DstBuffer, 1, &copyRegion);
    }

    VulkanVertexBuffer::VulkanVertexBuffer(VmaAllocator Allocator, const VertexBufferDesc &desc)
    {
        this->_Count = desc.Count;
        this->_Size = desc.SizeInBytes;

        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = desc.SizeInBytes;

        VmaAllocationCreateInfo allocInfo = {};

        if (desc.Type == BufferTypes::STATIC_DRAW)
        {
            // TODO: Use staging buffer VMA_MEMORY_USAGE_GPU_ONLY
            bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY; // For frequent updates
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        }
        else if (desc.Type == BufferTypes::DYNAMIC_DARW)
        {
            // Use staging buffer
            bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU; // For frequent updates
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        }
        else if (desc.Type == BufferTypes::DYNAMIC_STREAM)
        {
            // Use staging buffer
            bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU; // For frequent updates
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        }

        vmaCreateBuffer(Allocator, &bufferInfo, &allocInfo, &_Buffer, &_Allocation, &_AllocatoinInfo);
    }

    void VulkanVertexBuffer::UploadData(const void *data, uint32_t size)
    {
        memcpy(_AllocatoinInfo.pMappedData, data, size);
    }

    void VulkanVertexBuffer::Destroy(VmaAllocator Allocator)
    {
        vmaDestroyBuffer(Allocator, _Buffer, _Allocation);
    }

    VulkanIndexBuffer::VulkanIndexBuffer(VmaAllocator Allocator, const IndexBufferDesc &desc)
    {
        this->_Count = desc.Count;
        this->_Size = desc.SizeInBytes;
        this->_DataType = desc.DataType;

        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = desc.SizeInBytes;
        bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

        VmaAllocationCreateInfo allocInfo = {};

        if (desc.Type == BufferTypes::STATIC_DRAW)
        {
            // TODO: Use staging buffer VMA_MEMORY_USAGE_GPU_ONLY
            bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
            allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY; // For frequent updates
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }
        else if (desc.Type == BufferTypes::DYNAMIC_DARW)
        {
            // Use staging buffer
            bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
            allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU; // For frequent updates
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        }
        else if (desc.Type == BufferTypes::DYNAMIC_STREAM)
        {
            // Use staging buffer
            bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
            allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU; // For frequent updates
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        }

        vmaCreateBuffer(Allocator, &bufferInfo, &allocInfo, &_Buffer, &_Allocation, &_AllocatoinInfo);
    }

    void VulkanIndexBuffer::UploadData(const void *data, uint32_t size)
    {
        memcpy(_AllocatoinInfo.pMappedData, data, size);
    }

    void VulkanIndexBuffer::Destroy(VmaAllocator Allocator)
    {
        vmaDestroyBuffer(Allocator, _Buffer, _Allocation);
    }

    void _ReadFile(const char *FilePath, std::vector<char> &CharData)
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

    VkShaderModule _CreateShaderModule(VkDevice device, std::vector<char> &code)
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

    VkPipelineShaderStageCreateInfo _CreateShaderStages(VkShaderModule module, VkShaderStageFlagBits type, const char *Name)
    {
        VkPipelineShaderStageCreateInfo stage{};
        stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stage.module = module;
        stage.pName = Name;
        stage.stage = type;

        return stage;
    }

    size_t ShaderTypeToSize(ShaderDataType Type)
    {
        switch (Type)
        {
        case ShaderDataType::FLOAT1:
            return sizeof(float) * 1;
        case ShaderDataType::FLOAT2:
            return sizeof(float) * 2;
        case ShaderDataType::FLOAT3:
            return sizeof(float) * 3;
        case ShaderDataType::FLOAT4:
            return sizeof(float) * 4;
        case ShaderDataType::INT1:
            return sizeof(int) * 1;
        case ShaderDataType::INT2:
            return sizeof(int) * 2;
        case ShaderDataType::INT3:
            return sizeof(int) * 3;
        case ShaderDataType::INT4:
            return sizeof(int) * 4;
        case ShaderDataType::UINT1:
            return sizeof(unsigned int) * 1;
        case ShaderDataType::UINT2:
            return sizeof(unsigned int) * 2;
        case ShaderDataType::UINT3:
            return sizeof(unsigned int) * 3;
        case ShaderDataType::UINT4:
            return sizeof(unsigned int) * 4;
        default:
            break;
        }
    }

    VkFormat ShaderTypeToFormat(ShaderDataType Type)
    {
        switch (Type)
        {
        case ShaderDataType::FLOAT1:
            return VkFormat::VK_FORMAT_R32_SFLOAT;
        case ShaderDataType::FLOAT2:
            return VkFormat::VK_FORMAT_R32G32_SFLOAT;
        case ShaderDataType::FLOAT3:
            return VkFormat::VK_FORMAT_R32G32B32_SFLOAT;
        case ShaderDataType::FLOAT4:
            return VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT;
        case ShaderDataType::INT1:
            return VkFormat::VK_FORMAT_R32_SINT;
        case ShaderDataType::INT2:
            return VkFormat::VK_FORMAT_R32G32_SINT;
        case ShaderDataType::INT3:
            return VkFormat::VK_FORMAT_R32G32B32_SINT;
        case ShaderDataType::INT4:
            return VkFormat::VK_FORMAT_R32G32B32A32_SINT;
        case ShaderDataType::UINT1:
            return VkFormat::VK_FORMAT_R32_UINT;
        case ShaderDataType::UINT2:
            return VkFormat::VK_FORMAT_R32G32_UINT;
        case ShaderDataType::UINT3:
            return VkFormat::VK_FORMAT_R32G32B32_UINT;
        case ShaderDataType::UINT4:
            return VkFormat::VK_FORMAT_R32G32B32A32_UINT;
        }
    }

    VkVertexInputAttributeDescription GetAttributeDescription(int Binding, int Location, int Offset, VkFormat Format)
    {
        VkVertexInputAttributeDescription attributeDescriptions{};
        attributeDescriptions.binding = Binding;
        attributeDescriptions.location = Location;
        attributeDescriptions.format = Format; // vec3
        attributeDescriptions.offset = Offset;
        return attributeDescriptions;
    }

    void VulkanGraphicsPipeline::Init(const VulkanGraphicsPipelineSpec &Spec)
    {
        enum ShaderTypes
        {
            VERTEX,
            FRAGMENT,
            COUNT
        };

        VkShaderModule ShaderModules[ShaderTypes::COUNT];
        VkPipelineShaderStageCreateInfo ShaderStages[ShaderTypes::COUNT];

        std::vector<char> Sources[ShaderTypes::COUNT];
        _ReadFile(Spec.Paths[VERTEX], Sources[VERTEX]);
        _ReadFile(Spec.Paths[FRAGMENT], Sources[FRAGMENT]);

        ShaderModules[VERTEX] = _CreateShaderModule(Spec.device, Sources[VERTEX]);
        ShaderModules[FRAGMENT] = _CreateShaderModule(Spec.device, Sources[FRAGMENT]);

        ShaderStages[VERTEX] = _CreateShaderStages(ShaderModules[VERTEX], VK_SHADER_STAGE_VERTEX_BIT, "main");
        ShaderStages[FRAGMENT] = _CreateShaderStages(ShaderModules[FRAGMENT], VK_SHADER_STAGE_FRAGMENT_BIT, "main");

        struct BindingDataSpec
        {
            int Stride, Binding;

            BindingDataSpec(int pStride, int pBinding) : Stride(pStride), Binding(pBinding) {}
        };

        std::vector<BindingDataSpec> BindingData;
        std::vector<VkVertexInputAttributeDescription> AttributeDescriptions;
        AttributeDescriptions.reserve(Spec.Attributes.size());

        // Calculate Stride
        size_t Stride = 0;
        for (auto &attrib : Spec.Attributes)
            Stride += ShaderTypeToSize(attrib.Type);

        int Offset = 0;
        for (auto &attrib : Spec.Attributes)
        {
            AttributeDescriptions.push_back(GetAttributeDescription(attrib.Binding, attrib.Location, Offset, ShaderTypeToFormat(attrib.Type)));
            Offset += ShaderTypeToSize(attrib.Type);

            if (BindingData.size() == 0)
                BindingData.push_back({ShaderTypeToSize(attrib.Type), attrib.Binding});
            else
            {
                bool FoundBinding = false;
                for (auto &BindData : BindingData)
                    if (BindData.Binding == attrib.Binding)
                    {
                        BindData.Stride += ShaderTypeToSize(attrib.Type);
                        FoundBinding = true;
                    }

                if (!FoundBinding)
                    BindingData.push_back({ShaderTypeToSize(attrib.Type), attrib.Binding});
            }
        }

        // 🆕 UPDATED: Vertex input descriptions for the new shader
        std::vector<VkVertexInputBindingDescription> bindingDescriptions;
        bindingDescriptions.reserve(BindingData.size());

        for (auto &BindData : BindingData)
        {
            VkVertexInputBindingDescription bindingdesc{};
            bindingdesc.binding = BindData.Binding;
            bindingdesc.stride = BindData.Stride;
            bindingdesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            bindingDescriptions.push_back(bindingdesc);
        }

        // Pipeline layout
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &Spec.DescLayout;
        VULKAN_SUCCESS_ASSERT(vkCreatePipelineLayout(Spec.device, &pipelineLayoutInfo, nullptr, &_PipelineLayout), "[VULKAN]: Pipeline Layout creation failed!");

        // 🆕 NEW: Add dynamic rendering structure
        VkPipelineRenderingCreateInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachmentFormats = &Spec.SwapChainFormat; // Use swapchain format

        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR};

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

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

        // Graphics pipeline
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

        VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = bindingDescriptions.size();
        vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
        vertexInputInfo.vertexAttributeDescriptionCount = AttributeDescriptions.size();
        vertexInputInfo.pVertexAttributeDescriptions = AttributeDescriptions.data();

        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pNext = &renderingInfo; // Link dynamic rendering info
        pipelineInfo.stageCount = ShaderTypes::COUNT;
        pipelineInfo.pStages = ShaderStages;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = nullptr; // Optional
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.layout = _PipelineLayout;
        pipelineInfo.pDepthStencilState = nullptr; // Optional
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineInfo.basePipelineIndex = -1;

        // Create pipeline!
        VULKAN_SUCCESS_ASSERT(vkCreateGraphicsPipelines(Spec.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_Pipeline), "Graphics pipeline Failed to create!");

        // Cleanup shaders
        for (int i = 0; i < ShaderTypes::COUNT; i++)
            vkDestroyShaderModule(Spec.device, ShaderModules[i], nullptr);
    }

    void VulkanGraphicsPipeline::Destroy(VkDevice device)
    {
        vkDestroyPipeline(device, _Pipeline, nullptr);
        vkDestroyPipelineLayout(device, _PipelineLayout, nullptr);
    }

    VulkanUniformBuffer::VulkanUniformBuffer(VmaAllocator Allocator, const UniformBufferDesc &desc)
    {
        _Size = desc.Size;

        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = desc.Size;
        bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                          VMA_ALLOCATION_CREATE_MAPPED_BIT;
        allocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        // Use staging buffer
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU; // For frequent updates
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

        vmaCreateBuffer(Allocator, &bufferInfo, &allocInfo, &_Buffer, &_Allocation, &_AllocatoinInfo);
    }

    void VulkanUniformBuffer::UploadData(const void *data, uint32_t size)
    {
        memcpy(_AllocatoinInfo.pMappedData, data, size);
    }

    void VulkanUniformBuffer::Destroy(VmaAllocator Allocator)
    {
        vmaDestroyBuffer(Allocator, _Buffer, _Allocation);
    }

}
