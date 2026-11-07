#pragma once

#include <Duck/Core/Ray.h>

#include <cmath>

namespace Duck
{
    // pinhole camera, right handed, looks toward the target (down -z in local space)
    class Camera
    {
    public:
        Camera(const Vec3& position, const Vec3& target, const Vec3& up, float verticalFovDegrees, float aspectRatio)
        {
            const float viewportHeight = 2.0f * std::tan(Radians(verticalFovDegrees) * 0.5f);
            const float viewportWidth = aspectRatio * viewportHeight;

            //camera basis: w points backward (away from the target), u right, v up.
            const Vec3 w = glm::normalize(position - target);
            const Vec3 u = glm::normalize(glm::cross(up, w));
            const Vec3 v = glm::cross(w, u);

            origin = position;
            horizontal = viewportWidth * u;
            vertical = viewportHeight * v;
            // viewport sits one unit in front of the camera (focal length)
            upperLeft = position - 0.5f * this->horizontal + 0.5f * this->vertical - w;
        }

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