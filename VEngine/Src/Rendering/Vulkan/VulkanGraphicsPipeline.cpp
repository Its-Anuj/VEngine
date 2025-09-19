#include "VeVPCH.h"
#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan.h"
#include "VulkanRenderer.h"
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
} // namespace VEngine
