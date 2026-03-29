#include "log/logging.h"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>

#include "concurrentqueue.h"

namespace VisionCursor
{
namespace Log
{
namespace
{
constexpr std::size_t kMaxBufferedLines = 2000;

std::atomic<bool> g_inited{false};
std::atomic<bool> g_console{VC_LOG_DEFAULT_CONSOLE != 0};
moodycamel::ConcurrentQueue<std::string> g_lines;
std::atomic<std::size_t> g_line_count{0};
std::string g_app_name = "VisionCursor";

const char* toString(Severity severity)
{
    switch (severity)
    {
        case Severity::INFO: return "INFO";
        case Severity::WARNING: return "WARNING";
        case Severity::ERROR: return "ERROR";
        case Severity::FATAL: return "FATAL";
        default: return "UNKNOWN";
    }
}

Severity fromName(const char* name)
{
    if (name == nullptr)
    {
        return Severity::INFO;
    }

    const std::string s(name);
    if (s == "INFO")
    {
        return Severity::INFO;
    }
    if (s == "WARNING")
    {
        return Severity::WARNING;
    }
    if (s == "ERROR")
    {
        return Severity::ERROR;
    }
    if (s == "FATAL")
    {
        return Severity::FATAL;
    }
    return Severity::INFO;
}

std::string nowText()
{
    const auto now = std::chrono::system_clock::now();
    const auto tt = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif

    std::ostringstream os;
    os << std::put_time(&tm, "%H:%M:%S");
    return os.str();
}
} // namespace

bool Init(const std::string& app_name, bool enable_console)
{
    if (!app_name.empty())
    {
        g_app_name = app_name;
    }
    g_console.store(enable_console);
    g_inited.store(true);
    return true;
}

Severity SeverityFromName(const char* name)
{
    return fromName(name);
}

void Shutdown()
{
    g_inited.store(false);
}

bool IsInitialized()
{
    return g_inited.load();
}

void SetConsoleEnabled(bool enabled)
{
    g_console.store(enabled);
}

bool IsConsoleEnabled()
{
    return g_console.load();
}

void Push(Severity severity, const char* file, int line, const std::string& message)
{
    if (!g_inited.load())
    {
        Init();
    }
    (void)file;
    (void)line;

    std::ostringstream os;
    os << "[" << nowText() << "] "
       << "[" << toString(severity) << "] "
       << message;
    const std::string line_text = os.str();

    if (g_line_count.load(std::memory_order_relaxed) >= kMaxBufferedLines)
    {
        std::string dropped;
        if (g_lines.try_dequeue(dropped))
        {
            g_line_count.fetch_sub(1, std::memory_order_relaxed);
        }
    }
    g_lines.enqueue(line_text);
    g_line_count.fetch_add(1, std::memory_order_relaxed);

    if (g_console.load())
    {
        std::cout << line_text << std::endl;
    }

    if (severity == Severity::FATAL)
    {
        std::abort();
    }
}

std::vector<std::string> ConsumeLines(std::size_t max_lines)
{
    std::vector<std::string> out;
    if (max_lines == 0)
    {
        return out;
    }

    out.reserve(max_lines);
    std::string line;
    while (out.size() < max_lines && g_lines.try_dequeue(line))
    {
        out.push_back(std::move(line));
        g_line_count.fetch_sub(1, std::memory_order_relaxed);
    }
    return out;
}

void Clear()
{
    std::string dropped;
    while (g_lines.try_dequeue(dropped))
    {
    }
    g_line_count.store(0, std::memory_order_relaxed);
}

LogMessage::LogMessage(Severity severity, const char* file, int line) : severity_(severity), file_(file), line_(line) {}

LogMessage::~LogMessage()
{
    Push(severity_, file_, line_, stream_.str());
}

std::ostringstream& LogMessage::stream()
{
    return stream_;
}

} // namespace Log
} // namespace VisionCursor
