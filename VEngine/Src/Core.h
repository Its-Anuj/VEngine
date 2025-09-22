#pragma once

namespace VEngine
{
    template <typename T>
    using Scope = std::shared_ptr<T>;

    template <typename T>
    using Ref = std::shared_ptr<T>;
} // namespace VEngine
