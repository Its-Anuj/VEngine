#pragma once

#include "Shaders.h"
#include "VulkanRenderPass.h"

namespace VEngine
{
    class VulkanShader : public Shader
    {
    public:
        VulkanShader() {}
        ~VulkanShader() {}

        bool Init(const ShaderSpec &Spec) override;
        void Destroy() override;

        std::vector<VkPipelineShaderStageCreateInfo> &GetShaderStages() { return _ShaderStages; }
        VkPipelineVertexInputStateCreateInfo GetInfo(std::vector<VkVertexInputAttributeDescription> &attribdes, VkVertexInputBindingDescription &bindingDes) const;

    private:
        VkShaderModule _CreateShader(VkDevice device, const std::vector<char> &code);
        void _ReadFile(const char *FilePath, std::vector<char> &CharData);
        VkPipelineShaderStageCreateInfo _CreateShaderStages(VkShaderModule module, ShaderType type, const char *Name);
        void _CreatePipeline();

        ShaderSpec _Spec;
        std::vector<VkPipelineShaderStageCreateInfo> _ShaderStages;
        VkShaderModule ShaderModules[ShaderType::SHDAER_TYPE_COUNT];

        VkPipeline _Pipeline;
        VkPipelineLayout _PipelineLayout;
    };
} // namespace VEngine
