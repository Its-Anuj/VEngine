#pragma once

#include "UUID/UUID.h"

namespace VEngine
{
    enum class IndexBufferType
    {
        UINT_8,
        UINT_16,
        UINT_32,
        UINT_64,
    };

    enum class BufferTypes
    {
        STATIC,
        DYNAMIC
    };

    struct VertexBufferDesc
    {
        BufferTypes Type;
        void* Data;
        int SizeInBytes;
    };
    
    struct IndexBufferDesc
    {
        BufferTypes Type;
        void* Data;
        int SizeInBytes;
        IndexBufferType IntType;
    };

    class VertexBuffer
    {
    public:
        VertexBuffer() {}
        ~VertexBuffer() {}

        virtual bool Init(const VertexBufferDesc& desc) = 0;
        virtual void Bind() = 0;
        virtual void Destroy() = 0;

        static std::shared_ptr<VertexBuffer> Create(const VertexBufferDesc& desc);

        const UUID &ID() const { return _ID; }

    private:
        UUID _ID;
    };

    class IndexBuffer
    {
    public:
        IndexBuffer() {}
        ~IndexBuffer() {}

        virtual bool Init(const IndexBufferDesc& desc) = 0;
        virtual void Bind() = 0;
        virtual void Destroy() = 0;
        virtual uint32_t Size() const = 0;

        const UUID &ID() const { return _ID; }
        static std::shared_ptr<IndexBuffer> Create(const IndexBufferDesc& desc);

    private:
        UUID _ID;
    };
} // namespace VEngine
