#pragma once
#include <format>
#include <string_view>
#include <utility>

namespace Duck::Log {

    enum class Level { Trace, Debug, Info, Warn, Error, Fatal };

    void Write(Level level, std::string_view message);

    template <typename... TArgs>
    void Print(Level level, std::format_string<TArgs...> fmt, TArgs&&... args) {
        Write(level, std::format(fmt, std::forward<TArgs>(args)...));
    }

    template <typename... TArgs>
    void Trace(std::format_string<TArgs...> fmt, TArgs&&... args) { Print(Level::Trace, fmt, std::forward<TArgs>(args)...); }
    template <typename... TArgs>
    void Debug(std::format_string<TArgs...> fmt, TArgs&&... args) { Print(Level::Debug, fmt, std::forward<TArgs>(args)...); }
    template <typename... TArgs>
    void Info(std::format_string<TArgs...> fmt, TArgs&&... args) { Print(Level::Info, fmt, std::forward<TArgs>(args)...); }
    template <typename... TArgs>
    void Warn(std::format_string<TArgs...> fmt, TArgs&&... args) { Print(Level::Warn, fmt, std::forward<TArgs>(args)...); }
    template <typename... TArgs>
    void Error(std::format_string<TArgs...> fmt, TArgs&&... args) { Print(Level::Error, fmt, std::forward<TArgs>(args)...); }
    template <typename... TArgs>
    void Fatal(std::format_string<TArgs...> fmt, TArgs&&... args) { Print(Level::Fatal, fmt, std::forward<TArgs>(args)...); }

}