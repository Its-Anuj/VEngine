#include "VeVPCH.h"
#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan.h"
#include "VulkanRenderer.h"
#include "VulkanSwapChain.h"
#include "VulkanGraphicsPipeline.h"
#include "VulkanDevice.h"
#include "VulkanShaders.h"

#define PRINT(x)        \
    {                   \
        std::cout << x; \
    }
#define PRINTLN(x)              \
    {                           \
        std::cout << x << "\n"; \
    }

namespace VEngine
{

    bool VulkanShader::Init(const ShaderSpec &Spec)
    {
        auto device = GetRuntimeData()->ActiveDevice->GetHandle();

        std::vector<char> Sources[Spec.Paths.size()];

        for (int i = 0; i < Spec.Paths.size(); i++)
            _ReadFile(Spec.Paths[i].c_str(), Sources[i]);

        for (int i = 0; i < ShaderType::SHDAER_TYPE_COUNT; i++)
            ShaderModules[i] = nullptr;

        for (int i = 0; i < Spec.Paths.size(); i++)
            ShaderModules[Spec.UsingTypes[i]] = _CreateShader(device, Sources[i]);

        _Spec = Spec;

        _ShaderStages.reserve(Spec.Paths.size());

        for (int i = 0; i < Spec.Paths.size(); i++)
            _ShaderStages.push_back(_CreateShaderStages(ShaderModules[Spec.UsingTypes[i]], Spec.UsingTypes[i], Spec.Name.c_str()));

        return true;
    }

    void VulkanShader::Destroy()
    {
        auto device = GetRuntimeData()->ActiveDevice->GetHandle();
        for (auto module : ShaderModules)
            if (module != nullptr)
                vkDestroyShaderModule(device, module, nullptr);
    }

    int VertexTypesToSize(ShaderVertexTypes type)
    {
        switch (type)
        {
        case ShaderVertexTypes::FLOAT1:
            return sizeof(float) * 1;
        case ShaderVertexTypes::FLOAT2:
            return sizeof(float) * 2;
        case ShaderVertexTypes::FLOAT3:
            return sizeof(float) * 3;
        case ShaderVertexTypes::FLOAT4:
            return sizeof(float) * 4;
        case ShaderVertexTypes::INT1:
            return sizeof(int) * 1;
        case ShaderVertexTypes::INT2:
            return sizeof(int) * 2;
        case ShaderVertexTypes::INT3:
            return sizeof(int) * 3;
        case ShaderVertexTypes::INT4:
            return sizeof(int) * 4;
        case ShaderVertexTypes::UINT1:
            return sizeof(unsigned int) * 1;
        case ShaderVertexTypes::UINT2:
            return sizeof(unsigned int) * 2;
        case ShaderVertexTypes::UINT3:
            return sizeof(unsigned int) * 3;
        case ShaderVertexTypes::UINT4:
            return sizeof(unsigned int) * 4;
        default:
            throw std::runtime_error("VertexType not supported!");
        }
    }

    VkFormat VertexTypeToFormat(ShaderVertexTypes type)
    {
        switch (type)
        {
        case ShaderVertexTypes::FLOAT1:
            return VkFormat::VK_FORMAT_R32_SFLOAT;
        case ShaderVertexTypes::FLOAT2:
            return VkFormat::VK_FORMAT_R32G32_SFLOAT;
        case ShaderVertexTypes::FLOAT3:
            return VkFormat::VK_FORMAT_R32G32B32_SFLOAT;
        case ShaderVertexTypes::FLOAT4:
            return VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT;
        case ShaderVertexTypes::INT1:
            return VkFormat::VK_FORMAT_R32_SINT;
        case ShaderVertexTypes::INT2:
            return VkFormat::VK_FORMAT_R32G32_SINT;
        case ShaderVertexTypes::INT3:
            return VkFormat::VK_FORMAT_R32G32B32_SINT;
        case ShaderVertexTypes::INT4:
            return VkFormat::VK_FORMAT_R32G32B32A32_SINT;
        case ShaderVertexTypes::UINT1:
            return VkFormat::VK_FORMAT_R32_UINT;
        case ShaderVertexTypes::UINT2:
            return VkFormat::VK_FORMAT_R32G32_UINT;
        case ShaderVertexTypes::UINT3:
            return VkFormat::VK_FORMAT_R32G32B32_UINT;
        case ShaderVertexTypes::UINT4:
            return VkFormat::VK_FORMAT_R32G32B32A32_UINT;
        default:
            throw std::runtime_error("Format not supported!");
        }
    }

    VkVertexInputAttributeDescription getAttributeDescriptions(int binding, int location, ShaderVertexTypes Type, int Offset)
    {
        VkVertexInputAttributeDescription attributeDescriptions{};
        attributeDescriptions.binding = binding;
        attributeDescriptions.location = location;
        attributeDescriptions.format = VertexTypeToFormat(Type);
        attributeDescriptions.offset = Offset;

        return attributeDescriptions;
    }

    VkVertexInputBindingDescription getBindingDescription(int SetBinding, int Stride, VkVertexInputRate InputRate)
    {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = SetBinding;
        bindingDescription.stride = Stride;
        bindingDescription.inputRate = InputRate;

        return bindingDescription;
    }

    VkPipelineVertexInputStateCreateInfo VulkanShader::GetInfo(std::vector<VkVertexInputAttributeDescription> &attribdes, VkVertexInputBindingDescription &bindingDes) const
    {
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        int Stride = 0;
        for (auto &attrib : _Spec.Attribs)
            Stride += VertexTypesToSize(attrib.Type);

        attribdes.reserve(_Spec.Attribs.size());
        int Offset = 0;

        for (auto &attrib : _Spec.Attribs)
        {
            attribdes.push_back(getAttributeDescriptions(attrib.binding, attrib.location, attrib.Type, Offset));
            Offset += VertexTypesToSize(attrib.Type);
        }

        // TODO: 0 is hardcoded!
        bindingDes = getBindingDescription(0, Stride, VK_VERTEX_INPUT_RATE_VERTEX);

        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribdes.size());
        vertexInputInfo.pVertexBindingDescriptions = &bindingDes;
        vertexInputInfo.pVertexAttributeDescriptions = attribdes.data();

        return vertexInputInfo;
    }

    VkShaderModule VulkanShader::_CreateShader(VkDevice device, const std::vector<char> &code)
    {
        VkShaderModuleCreateInfo createinfo;
        createinfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createinfo.codeSize = code.size();
        createinfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createinfo, nullptr, &shaderModule) != VK_SUCCESS)
            throw std::runtime_error("failed to create shader module!");

        return shaderModule;
    }

    void VulkanShader::_ReadFile(const char *FilePath, std::vector<char> &CharData)
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

    VkPipelineShaderStageCreateInfo VulkanShader::_CreateShaderStages(VkShaderModule module, ShaderType type, const char *Name)
    {
        VkPipelineShaderStageCreateInfo stage{};
        stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

        if (type == ShaderType::SHDAER_TYPE_VERTEX)
            stage.stage = VK_SHADER_STAGE_VERTEX_BIT;
        else if (type == ShaderType::SHDAER_TYPE_FRAGMENT)
            stage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        else if (type == ShaderType::SHDAER_TYPE_COMPUTE)
            stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;

        stage.module = module;
        stage.pName = Name;
        return stage;
    }

    void VulkanShader::_CreatePipeline()
    {
        auto Data = GetRuntimeData();
        auto device = GetRuntimeData()->ActiveDevice->GetHandle();

        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR};

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        std::vector<VkVertexInputAttributeDescription> attribdes;
        VkVertexInputBindingDescription bindingDes;

        VkPipelineVertexInputStateCreateInfo vertexInputInfo = GetInfo(attribdes, bindingDes);

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)Data->SwapChain->GetSwapChainExtent().width;
        viewport.height = (float)Data->SwapChain->GetSwapChainExtent().height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = Data->SwapChain->GetSwapChainExtent();

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
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

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

        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0;            // Optional
        pipelineLayoutInfo.pSetLayouts = nullptr;         // Optional
        pipelineLayoutInfo.pushConstantRangeCount = 0;    // Optional
        pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &_PipelineLayout) != VK_SUCCESS)
            throw std::runtime_error("failed to create pipeline layout!");

        auto shaderStages = GetShaderStages();
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
        pipelineInfo.layout = _PipelineLayout;
        // pipelineInfo.renderPass = _Data->RenderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
        pipelineInfo.basePipelineIndex = -1;              // Optional

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_Pipeline) != VK_SUCCESS)
            throw std::runtime_error("failed to create graphics pipeline!");

        PRINTLN("[VULKAN]: Graphics Pipeline Created!")

    }

    void VulkanRenderPassAttachment::Fill(const RenderPassAttachmentSpec &Spec)
    {
        auto Data = GetRuntimeData();
        std::vector<VkAttachmentDescription> Attachments;
        std::vector<VkAttachmentReference> AttachmentsRefs;
        std::vector<VkSubpassDescription> Subpasses;



    }
} // namespace VEngine
