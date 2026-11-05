#pragma once
#include <Duck/Core/Log.h>

#include <cstdlib>
#include <format>
#include <source_location>
#include <string>
#include <string_view>
#include <utility>

#if defined(_MSC_VER)
#define DUCK_DEBUG_BREAK() __debugbreak()
#else
#define DUCK_DEBUG_BREAK() __builtin_trap()
#endif

namespace Duck::Detail
{
	//logs failure with location and stack trace
	void ReportAssertFailure(const char* expression, std::source_location location, std::string_view message);

	inline std::string FormatAssertMessage() { return {}; }

	template <typename... TArgs>
	std::string FormatAssertMessage(std::format_string<TArgs...> fmt, TArgs&&... args)
	{
		return std::format(fmt, std::forward<TArgs>(args)...);
	}
}

#define DUCK_VERIFY(condition, ...)                                                            \
    do {                                                                                  \
        if (!(condition)) [[unlikely]] {                                                  \
            ::Duck::Detail::ReportAssertFailure(                                          \
                #condition, std::source_location::current(),                              \
                ::Duck::Detail::FormatAssertMessage(__VA_ARGS__));                        \
            DUCK_DEBUG_BREAK();                                                                \
            std::abort();                                                                 \
        }                                                                                 \
    } while(false)

#if !defined(NDEBUG) || defined(DUCK_FORCE_ASSERTS)
#define DUCK_ASSERT(condition, ...) DUCK_VERIFY(condition, __VA_ARGS__)
#else
#define DUCK_ASSERT(condition, ...) do{(void)sizeof(condition);} while(false)
#endif