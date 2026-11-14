#pragma once

#include <string>
#include <string_view>
#include <span>

namespace Duck::Console
{
    void WriteLine(std::string_view line);

    void SetStatus(std::span<const std::string> lines);

    void ClearStatus();

    bool SupportsEscapes();
}