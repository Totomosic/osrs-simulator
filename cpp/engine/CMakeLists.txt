add_library(osrssim_engine
  src/Engine.cpp
)

add_library(osrssim::engine ALIAS osrssim_engine)

target_include_directories(osrssim_engine
  PUBLIC
    src
)

target_link_libraries(osrssim_engine
  PUBLIC
    osrssim::common
)
