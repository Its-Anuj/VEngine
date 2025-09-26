#pragma once

#include <glm/glm.hpp>
#include "UUID/UUID.h"

namespace VEngine
{
    struct Vertex
    {
        glm::vec2 pos;
        glm::vec3 color;
        glm::vec2 tex;
    };

    enum class IndexBufferType
    {
        UINT_8,
        UINT_16,
        UINT_32,
        UINT_64,
    };

    enum class BufferTypes
    {
        STATIC_DRAW,
        DYNAMIC_DARW,
        DYNAMIC_STREAM,
    };

    struct VertexBufferDesc
    {
        BufferTypes Type;
        void *Data;
        int SizeInBytes;
        int32_t Count = 0;
    };

    struct IndexBufferDesc
    {
        BufferTypes Type;
        void *Data;
        int SizeInBytes;
        int32_t Count = 0;
        IndexBufferType DataType;
    };

    class VertexBuffer
    {
    public:
        VertexBuffer() {}
        virtual ~VertexBuffer() {}

        virtual void UploadData(const void *data, uint32_t size) = 0;

        const UUID &ID() const { return _ID; }
        uint32_t GetSize() const { return _Size; }
        int32_t GetCoutn() const { return _Count; }

    protected:
        UUID _ID;
        // Size in bytes of the buffer
        uint32_t _Size = 0;
        int32_t _Count = 0;
    };

    class IndexBuffer
    {
    public:
        IndexBuffer() {}
        virtual ~IndexBuffer() {}

        virtual void UploadData(const void *data, uint32_t size) = 0;

        const UUID &ID() const { return _ID; }
        uint32_t GetSize() const { return _Size; }
        int32_t GetCount() const { return _Count; }
        IndexBufferType GetDataType() const { return _DataType; }

    protected:
        UUID _ID;
        // Size in bytes of the buffer
        uint32_t _Size = 0;
        int32_t _Count = 0;
        IndexBufferType _DataType;
    };
} // namespace VEngine
