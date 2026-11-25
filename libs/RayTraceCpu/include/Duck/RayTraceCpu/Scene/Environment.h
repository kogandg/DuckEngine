#pragma once

#include <Duck/Core/Math.h>
#include <Duck/RayTraceCpu/Spectrum.h>

namespace Duck::RayTraceCpu
{
    // light from far away: the background rays see when they escape, and Whitteds ambient approximation
    struct Environment
    {
        // radiance of the background, W/(m^2 sr): a gradient from straight down to straight up
        Rgb SkyDown{1.0f};
        Rgb SkyUp{0.5f, 0.7f, 1.0f};

        // Whitted ambient: radiance of a surrounding environment, ignoring occlusion
        Rgb AmbientDown{0.3f};
        Rgb AmbientUp{0.15f, 0.21f, 0.3f};

        Spectrum SkyColor(const Vec3& direction) const { return glm::mix(SkyDown, SkyUp, 0.5f * (direction.y + 1.0f)); }
        Spectrum Ambient(const Vec3& normal) const
        {
            return glm::mix(AmbientDown, AmbientUp, 0.5f * (normal.y + 1.0f));
        }
    };
}