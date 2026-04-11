#include "SHE/Core/Logger.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <mutex>

namespace she
{
namespace
{
std::mutex g_logMutex;

const char* ToString(const LogLevel level)
{
    switch (level)
    {
    case LogLevel::Trace:
        return "Trace";
    case LogLevel::Info:
        return "Info";
    case LogLevel::Warning:
        return "Warning";
    case LogLevel::Error:
        return "Error";
    default:
        return "Unknown";
    }
}
} // namespace

void Logger::Write(const LogLevel level, const std::string_view category, const std::string_view message)
{
    const auto now = std::chrono::system_clock::now();
    const std::time_t nowTime = std::chrono::system_clock::to_time_t(now);

    std::tm localTime{};
#if defined(_WIN32)
    localtime_s(&localTime, &nowTime);
#else
    localtime_r(&nowTime, &localTime);
#endif

    std::lock_guard lock(g_logMutex);
    std::cout
        << '[' << std::put_time(&localTime, "%H:%M:%S") << ']'
        << '[' << ToString(level) << ']'
        << '[' << category << "] "
        << message
        << '\n';
}
} // namespace she

