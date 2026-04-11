#pragma once

#include <string_view>

namespace she
{
enum class LogLevel
{
    Trace,
    Info,
    Warning,
    Error
};

class Logger
{
public:
    static void Write(LogLevel level, std::string_view category, std::string_view message);
};
} // namespace she

#define SHE_LOG_TRACE(category, message) ::she::Logger::Write(::she::LogLevel::Trace, category, message)
#define SHE_LOG_INFO(category, message) ::she::Logger::Write(::she::LogLevel::Info, category, message)
#define SHE_LOG_WARNING(category, message) ::she::Logger::Write(::she::LogLevel::Warning, category, message)
#define SHE_LOG_ERROR(category, message) ::she::Logger::Write(::she::LogLevel::Error, category, message)

