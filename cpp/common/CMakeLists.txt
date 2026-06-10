add_library(osrssim_common
  src/types.cpp
)

add_library(osrssim::common ALIAS osrssim_common)

target_include_directories(osrssim_common
  PUBLIC
    src
)
