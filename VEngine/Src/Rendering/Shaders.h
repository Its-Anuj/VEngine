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

    enum class ShaderVertexTypes
    {
        FLOAT1,
        FLOAT2,
        FLOAT3,
        FLOAT4,
        INT1,
        INT2,
        INT3,
        INT4,
        UINT1,
        UINT2,
        UINT3,
        UINT4,
    };

    struct VertexBufferAttrib
    {
        const char *Name;
        ShaderVertexTypes Type;
        int binding = 0;
        int location = 0;
    };

    struct ShaderSpec
    {
        std::vector<std::string> Paths;
        std::vector<ShaderType> UsingTypes;
        std::vector<VertexBufferAttrib> Attribs;
        std::string Name;
    };

    class Shader
    {
    public:
        Shader() {}
        virtual ~Shader() {}

        virtual bool Init(const ShaderSpec &Spec) = 0;
        virtual void Destroy() = 0;

    private:
    };
} // namespace VEngine
