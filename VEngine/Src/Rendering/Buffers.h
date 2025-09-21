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

    class VertexBuffer
    {
    public:
        VertexBuffer() {}
        ~VertexBuffer() {}

        virtual bool Init(float *data, int FloatCount, BufferTypes Type) = 0;
        virtual void Bind() = 0;
        virtual void Destroy() = 0;

        static std::shared_ptr<VertexBuffer> Create(float *vertices, uint32_t size, BufferTypes BType);

        const UUID &ID() const { return _ID; }

    private:
        UUID _ID;
    };

    class IndexBuffer
    {
    public:
        IndexBuffer() {}
        ~IndexBuffer() {}

        virtual bool Init(void *data, int Uintcount, IndexBufferType Type, BufferTypes BType) = 0;
        virtual void Bind() = 0;
        virtual void Destroy() = 0;
        virtual uint32_t Size() const = 0;

        const UUID &ID() const { return _ID; }
        static std::shared_ptr<IndexBuffer> Create(void *data, int Uintcount, IndexBufferType Type, BufferTypes BType);

    private:
        UUID _ID;
    };
} // namespace VEngine
