add_library(osrssim_common
  src/Logging.cpp
  src/Types.cpp
)

add_library(osrssim::common ALIAS osrssim_common)

target_include_directories(osrssim_common
  PUBLIC
    src
)

target_link_libraries(osrssim_common
  PUBLIC
    spdlog::spdlog
)

if(BUILD_TESTING)
  add_executable(osrssim_common_logging_smoke
    tests/LoggingSmoke.cpp
  )

  target_link_libraries(osrssim_common_logging_smoke
    PRIVATE
      osrssim::common
  )

  add_test(NAME osrssim_common_logging_smoke COMMAND osrssim_common_logging_smoke)
endif()
