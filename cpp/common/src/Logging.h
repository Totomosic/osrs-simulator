#pragma once

#include <cstdlib>
#include <memory>
#include <utility>

#include <spdlog/spdlog.h>

namespace osrssim
{

class Log
{
public:
    static void Init();
    static std::shared_ptr<spdlog::logger>& GetLogger();
    static void SetLevel(spdlog::level::level_enum level);
};

inline void LogAssert(bool condition, const char* expression)
{
    if (condition)
    {
        return;
    }

    SPDLOG_LOGGER_CRITICAL(Log::GetLogger(), "Assertion failed: {}", expression);
    std::abort();
}

template <typename... Args>
void LogAssert(bool condition, const char* expression, spdlog::format_string_t<Args...> format, Args&&... args)
{
    if (condition)
    {
        return;
    }

    SPDLOG_LOGGER_CRITICAL(Log::GetLogger(), "Assertion failed: {}", expression);
    SPDLOG_LOGGER_CRITICAL(Log::GetLogger(), format, std::forward<Args>(args)...);
    std::abort();
}

}  // namespace osrssim

#define LOG_TRACE(...) SPDLOG_LOGGER_TRACE(::osrssim::Log::GetLogger(), __VA_ARGS__)
#define LOG_DEBUG(...) SPDLOG_LOGGER_DEBUG(::osrssim::Log::GetLogger(), __VA_ARGS__)
#define LOG_INFO(...) SPDLOG_LOGGER_INFO(::osrssim::Log::GetLogger(), __VA_ARGS__)
#define LOG_WARN(...) SPDLOG_LOGGER_WARN(::osrssim::Log::GetLogger(), __VA_ARGS__)
#define LOG_ERROR(...) SPDLOG_LOGGER_ERROR(::osrssim::Log::GetLogger(), __VA_ARGS__)
#define LOG_ASSERT(condition, ...) \
    do \
    { \
        ::osrssim::LogAssert(static_cast<bool>(condition), #condition __VA_OPT__(, ) __VA_ARGS__); \
    } while (false)
