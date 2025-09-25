#pragma once

#include "Buffers.h"
#include "Shaders.h"

namespace VEngine
{
    class ResourceFactory
    {
    public:
        virtual ~ResourceFactory() {}
 
        virtual Ref<VertexBuffer> CreateVertexBuffer(const VertexBufferDesc& desc) = 0;
        virtual Ref<IndexBuffer> CreateIndexBuffer(const IndexBufferDesc& desc) = 0;
        
        virtual bool DeleteVertexBuffer(const Ref<VertexBuffer>& VB) = 0;
        virtual bool DeleteIndexBuffer(const Ref<IndexBuffer>& IB) = 0;

    private:
    };
} // namespace VEngine
