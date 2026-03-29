#pragma once

#include <cstddef>
#include <sstream>
#include <string>
#include <vector>

#ifdef ERROR
#undef ERROR
#endif

#ifndef VC_LOG_DEFAULT_CONSOLE
#define VC_LOG_DEFAULT_CONSOLE 0
#endif

namespace VisionCursor
{
namespace Log
{

enum class Severity
{
    INFO = 0,
    WARNING = 1,
    ERROR = 2,
    FATAL = 3
};

Severity SeverityFromName(const char* name);

bool Init(const std::string& app_name = "VisionCursor", bool enable_console = (VC_LOG_DEFAULT_CONSOLE != 0));
void Shutdown();
bool IsInitialized();

void SetConsoleEnabled(bool enabled);
bool IsConsoleEnabled();

void Push(Severity severity, const char* file, int line, const std::string& message);
std::vector<std::string> ConsumeLines(std::size_t max_lines = 256);
void Clear();

class LogMessage
{
public:
    LogMessage(Severity severity, const char* file, int line);
    ~LogMessage();

    std::ostringstream& stream();

private:
    Severity severity_;
    const char* file_;
    int line_;
    std::ostringstream stream_;
};

} // namespace Log
} // namespace VisionCursor

#define LOG(severity) \
    ::VisionCursor::Log::LogMessage(::VisionCursor::Log::SeverityFromName(#severity), __FILE__, __LINE__).stream()
