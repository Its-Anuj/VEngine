#include "VeVPCH.h"
#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan.h"
#include "VulkanGraphicsPipeline.h"
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

    bool VulkanShader::Init(VkDevice device, const VulkanShaderSpec &Spec)
    {
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

    void VulkanShader::Destroy(VkDevice device)
    {
        for (auto module : ShaderModules)
            if (module != nullptr)
                vkDestroyShaderModule(device, module, nullptr);
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
