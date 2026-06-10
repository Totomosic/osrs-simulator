add_executable(osrs-sim-cli
  Main.cpp
)

target_link_libraries(osrs-sim-cli
  PRIVATE
    osrssim::engine
    osrssim::common
)
