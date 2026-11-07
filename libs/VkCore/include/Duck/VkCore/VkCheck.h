#pragma once
#include <Duck/Core/Assert.h>

#include <volk.h>

// for vulkan calls that must succeed, not for handled results like VK_ERROR_OUT_OF_DATE_KHR
#define DUCK_VK_CHECK(expression)                                                                                      \
    do                                                                                                                 \
    {                                                                                                                  \
        const VkResult duckVkResult = (expression);                                                                    \
        DUCK_VERIFY(duckVkResult == VK_SUCCESS, "{} failed with VkResult {}", #expression,                             \
                    static_cast<int>(duckVkResult));                                                                   \
    } while (false)