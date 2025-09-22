#pragma once

#include "Buffers.h"
#include "Shaders.h"

namespace VEngine
{
    class ResourceFactory
    {
    public:
        virtual Ref<VertexBuffer> CreateVertexBuffer(const VertexBufferDesc& desc) = 0;
        virtual Ref<IndexBuffer> CreateIndexBuffer(const IndexBufferDesc& desc) = 0;
        virtual Ref<Shader> CreateGraphicsPipeline(const GraphicsShaderDesc& desc) = 0;

    private:
    };
} // namespace VEngine
