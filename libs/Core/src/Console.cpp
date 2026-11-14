#include <Duck/Core/Console.h>

#include <algorithm>
#include <cstdio>
#include <mutex>
#include <string>
#include <vector>

#ifdef _WIN32
#include <io.h>
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <unistd.h>
#endif

namespace Duck::Console
{
    namespace
    {
        struct State
        {
            std::mutex mutex;
            std::vector<std::string> status;
            int shownLines = 0;
            bool escapes = true;
        };

        bool EnableEscapes()
        {
#ifdef _WIN32
            if (!_isatty(_fileno(stdout)))
            {
                return false;
            }
            const HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
            DWORD mode = 0;
            if (output == INVALID_HANDLE_VALUE || !GetConsoleMode(output, &mode) ||
                !SetConsoleMode(output, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING))
            {
                return false; // older Windows: no escape codes
            }
            SetConsoleOutputCP(CP_UTF8); // the sources are UTF-8 (/utf-8)
            return true;
#else
            return isatty(fileno(stdout)) != 0;
#endif
        }

        State& GetState()
        {
            static State state;
            static const bool initialized = (state.escapes = EnableEscapes(), true);
            (void)initialized;
            return state;
        }

        int ConsoleWidth()
        {
#ifdef _WIN32
            CONSOLE_SCREEN_BUFFER_INFO info;
            if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info))
            {
                return info.srWindow.Right - info.srWindow.Left + 1;
            }
#else
            winsize size{};
            if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) == 0 && size.ws_col > 0)
            {
                return size.ws_col;
            }
#endif
            return 120;
        }

        // with the lock held: move back up to where the status block starts and erase everything below
        void EraseStatus(State& state, std::string& out)
        {
            if (state.shownLines > 0)
            {
                out += "\x1b[" + std::to_string(state.shownLines) + "F\x1b[0J";
                state.shownLines = 0;
            }
        }

        void DrawStatus(State& state, std::string& out)
        {
            // a line that wraps takes two rows, and erasing would then miss one: cut lines at the console width
            const size_t width = static_cast<size_t>(std::max(ConsoleWidth() - 1, 1));
            for (const std::string& line : state.status)
            {
                out.append(line, 0, std::min(line.size(), width));
                out += '\n';
            }
            state.shownLines = static_cast<int>(state.status.size());
        }

        // one write per update, so the block doesn't flicker
        void Flush(const std::string& out)
        {
            std::fwrite(out.data(), 1, out.size(), stdout);
            std::fflush(stdout);
        }
    }

    void WriteLine(std::string_view line)
    {
        State& state = GetState();
        std::lock_guard lock(state.mutex);
        std::string out;
        if (state.escapes)
        {
            EraseStatus(state, out);
        }
        out += line;
        out += '\n';
        if (state.escapes)
        {
            DrawStatus(state, out);
        }
        Flush(out);
    }

    void SetStatus(std::span<const std::string> lines)
    {
        State& state = GetState();
        if (!state.escapes)
        {
            return; // can't redraw in place: printing it every time would flood the output
        }
        std::lock_guard lock(state.mutex);
        std::string out;
        EraseStatus(state, out);
        state.status.assign(lines.begin(), lines.end());
        DrawStatus(state, out);
        Flush(out);
    }

    void ClearStatus()
    {
        State& state = GetState();
        std::lock_guard lock(state.mutex);
        std::string out;
        EraseStatus(state, out);
        state.status.clear();
        Flush(out);
    }

    bool SupportsEscapes() { return GetState().escapes; }
}