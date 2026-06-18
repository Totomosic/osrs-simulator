cmake_minimum_required(VERSION 3.24)

project(osrs_simulator
  VERSION 0.1.0
  DESCRIPTION "OSRS simulation and optimization toolkit"
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)
include(FetchContent)

find_package(spdlog 1.17.0 CONFIG QUIET)
find_package(nlohmann_json 3.11.3 CONFIG QUIET)

if(NOT spdlog_FOUND)
  set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
  set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(SPDLOG_BUILD_BENCH OFF CACHE BOOL "" FORCE)

  FetchContent_Declare(spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.17.0
    GIT_SHALLOW TRUE
  )

  FetchContent_MakeAvailable(spdlog)
endif()

if(NOT nlohmann_json_FOUND)
  FetchContent_Declare(nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW TRUE
  )

  FetchContent_MakeAvailable(nlohmann_json)
endif()

add_subdirectory(cpp)
