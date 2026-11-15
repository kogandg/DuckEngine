#pragma once

#include <Duck/Core/Math.h>

namespace Duck::RayTraceCpu
{
    // light from far away: the background rays see when they escape, and Whitteds ambient approximation
    struct Environment
    {
        // radiance of the background, W/(m^2 sr): a gradient from straight down to straight up
        Vec3 SkyDown{1.0f};
        Vec3 SkyUp{0.5f, 0.7f, 1.0f};

        // Whitted ambient: radiance of a surrounding environment, ignoring occlusion
        Vec3 AmbientDown{0.3f};
        Vec3 AmbientUp{0.15f, 0.21f, 0.3f};

        Vec3 SkyColor(const Vec3& direction) const { return glm::mix(SkyDown, SkyUp, 0.5f * (direction.y + 1.0f)); }
        Vec3 Ambient(const Vec3& normal) const { return glm::mix(AmbientDown, AmbientUp, 0.5f * (normal.y + 1.0f)); }
    };
}