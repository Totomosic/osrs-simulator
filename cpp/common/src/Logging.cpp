#include "Logging.h"

#include <memory>

#include <spdlog/sinks/stdout_color_sinks.h>

namespace osrssim
{
namespace
{

std::shared_ptr<spdlog::logger> CreateLogger()
{
    if (auto existingLogger = spdlog::get("osrssim"))
    {
        return existingLogger;
    }

    auto logger = spdlog::stdout_color_mt("osrssim");
    logger->set_pattern("[%H:%M:%S.%e] [%^%l%$] %v");
    logger->set_level(spdlog::level::info);
    return logger;
}

}  // namespace

void Log::Init()
{
    (void)GetLogger();
}

std::shared_ptr<spdlog::logger>& Log::GetLogger()
{
    static auto logger = CreateLogger();
    return logger;
}

void Log::SetLevel(spdlog::level::level_enum level)
{
    GetLogger()->set_level(level);
}

}  // namespace osrssim
