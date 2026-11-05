#include <Duck/Core/Assert.h>

#include <cstdio>
#include <stacktrace>

namespace Duck::Detail
{
	void ReportAssertFailure(const char* expression, std::source_location location, std::string_view message)
	{
		std::string text = std::format("Assertion failed: {}\n  at {}:{} ({})", expression, location.file_name(), location.line(), location.function_name());
		if (!message.empty()) 
		{
			text += std::format("\n  message: {}", message);
		}
		text += "\n";
		text += std::to_string(std::stacktrace::current(2));

		Log::Write(Log::Level::Fatal, text);
		std::fflush(stdout);
	}
}