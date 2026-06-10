add_executable(osrs-sim
  src/main.cpp
)

target_link_libraries(osrs-sim
  PRIVATE
    osrssim::sim
)
