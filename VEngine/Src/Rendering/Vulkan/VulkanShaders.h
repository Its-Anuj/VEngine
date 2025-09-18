#pragma once

namespace VEngine
{
    enum ShaderType
    {
        SHDAER_TYPE_VERTEX,
        SHDAER_TYPE_FRAGMENT,
        SHDAER_TYPE_COMPUTE,
        SHDAER_TYPE_COUNT
    };

    struct VulkanShaderSpec
    {
        std::vector<std::string> Paths;
        std::vector<ShaderType> UsingTypes;
        std::string Name;
    };

    class VulkanShader
    {
    public:
        VulkanShader() {}
        ~VulkanShader() {}

        bool Init(VkDevice device, const VulkanShaderSpec &Spec);
        void Destroy(VkDevice device);

        std::vector<VkPipelineShaderStageCreateInfo> &GetShaderStages() { return _ShaderStages; }

    private:
        VkShaderModule _CreateShader(VkDevice device, const std::vector<char> &code);
        void _ReadFile(const char *FilePath, std::vector<char> &CharData);
        VkPipelineShaderStageCreateInfo _CreateShaderStages(VkShaderModule module, ShaderType type, const char *Name);

        VulkanShaderSpec _Spec;
        std::vector<VkPipelineShaderStageCreateInfo> _ShaderStages;
        VkShaderModule ShaderModules[ShaderType::SHDAER_TYPE_COUNT];
    };
} // namespace VEngine
