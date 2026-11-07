#pragma once
#include <Duck/Core/Assert.h>

#include <utility>

namespace Duck
{
    template <typename TTo, typename TFrom>
    constexpr TTo CheckedCast(TFrom value)
    {
        DUCK_ASSERT(std::in_range<TTo>(value), "Value {} does not fit in the target type", value);
        return static_cast<TTo>(value);
    }
}