#include <Duck/PathTraceCpu/Camera.h>

#include <cmath>

namespace Duck::PathTrace
{
    Camera::Camera(const Vec3& position, const Vec3& target, const Vec3& up, float verticalFovDegrees,
                   float aspectRatio)
    {
        const float viewportHeight = 2.0f * std::tan(Radians(verticalFovDegrees) * 0.5f);
        const float viewportWidth = aspectRatio * viewportHeight;

        // camera basis: w points backward (away from the target), u right, v up
        const Vec3 w = glm::normalize(position - target);
        const Vec3 u = glm::normalize(glm::cross(up, w));
        const Vec3 v = glm::cross(w, u);

        origin = position;
        horizontal = viewportWidth * u;
        vertical = viewportHeight * v;
        // viewport sits one unit in front of the camera (focal length)
        upperLeft = position - 0.5f * this->horizontal + 0.5f * this->vertical - w;
    }
}