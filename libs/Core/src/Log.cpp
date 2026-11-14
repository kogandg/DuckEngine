#include <Duck/Core/Log.h>

#include <Duck/Core/Console.h>

#include <chrono>
#include <format>

namespace Duck::Log
{

    void Write(Level level, std::string_view message)
    {
        static constexpr const char* LevelNames[] = {"TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"};
        // gray for trace/debug, nothing for info, yellow warnings, red errors, bold red fatal
        static constexpr const char* LevelColors[] = {"\x1b[90m", "\x1b[90m", "\x1b[32m",
                                                      "\x1b[33m", "\x1b[31m", "\x1b[1;31m"};

        const auto now = std::chrono::floor<std::chrono::milliseconds>(std::chrono::system_clock::now());
        const std::chrono::zoned_time local{std::chrono::current_zone(), now};

        const int index = static_cast<int>(level);
        const bool color = Console::SupportsEscapes() && LevelColors[index][0] != '\0';
        Console::WriteLine(std::format("{:%H:%M:%S} [{}{}{}] {}", local, color ? LevelColors[index] : "",
                                       LevelNames[index], color ? "\x1b[0m" : "", message));
    }

}