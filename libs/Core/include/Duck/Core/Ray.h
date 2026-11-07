#pragma once

#include <Duck/Core/Math.h>

namespace Duck
{
    //direction is expected to be normalized
    struct Ray
    {
        Vec3 Origin{0.0f};
        Vec3 Direction{0.0f, 0.0f, -1.0f};

        Vec3 At(float t) const { return this->Origin + (t * this->Direction); }
    };
}