#include "Logging.h"

#include <spdlog/spdlog.h>

int main()
{
    osrssim::Log::SetLevel(spdlog::level::debug);

    LOG_DEBUG("debug smoke {}", 1);
    LOG_INFO("info smoke {}", 2);
    LOG_WARN("warn smoke {}", 3);
    LOG_ERROR("error smoke {}", 4);
    LOG_ASSERT(2 + 2 == 4, "assert smoke {}", 5);

    return 0;
}
