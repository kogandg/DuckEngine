#pragma once
#include <glm/glm.hpp>

namespace UBOData
{
    struct Camera
    {
        glm::mat4 view;
        glm::mat4 projection;
    };
};