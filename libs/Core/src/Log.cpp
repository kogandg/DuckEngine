#include <Duck/Core/Log.h>

#include <chrono>
#include <mutex>
#include <print>

namespace Duck::Log {

    void Write(Level level, std::string_view message) {
        static constexpr const char* LevelNames[] = { "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL" };
        static std::mutex mutex;

        const auto now = std::chrono::floor<std::chrono::milliseconds>(std::chrono::system_clock::now());
        const std::chrono::zoned_time local{ std::chrono::current_zone(), now };

        std::lock_guard lock(mutex);
        std::println("{:%H:%M:%S} [{}] {}", local, LevelNames[static_cast<int>(level)], message);
    }

}