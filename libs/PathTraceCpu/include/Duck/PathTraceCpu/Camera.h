#pragma once

#include <Duck/Core/Math.h>
#include <Duck/Core/Ray.h>

namespace Duck::PathTrace
{
    // pinhole camera, right handed, looks toward the target (down -z in local space)
    class Camera
    {
    public:
        Camera(const Vec3& position, const Vec3& target, const Vec3& up, float verticalFovDegrees, float aspectRatio);

        // s: 0 = left edge, 1 = right edge. t: 0 = TOP edge, 1 = bottom edge (matches framebuffer rows)
        Ray GetRay(float s, float t) const
        {
            const Vec3 point = upperLeft + s * horizontal - t * vertical;
            return {origin, glm::normalize(point - origin)};
        }

    private:
        Vec3 origin{0.0f};
        Vec3 upperLeft{0.0f};
        Vec3 horizontal{0.0f};
        Vec3 vertical{0.0f};
    };
}