add_library(osrssim_sim
  src/scenario.cpp
)

add_library(osrssim::sim ALIAS osrssim_sim)

target_include_directories(osrssim_sim
  PUBLIC
    src
)

target_link_libraries(osrssim_sim
  PUBLIC
    osrssim::engine
)

if(BUILD_TESTING)
  add_executable(osrssim_sim_tests
    tests/scenario_test.cpp
  )

  target_link_libraries(osrssim_sim_tests
    PRIVATE
      osrssim::sim
  )

  add_test(NAME osrssim_sim_tests COMMAND osrssim_sim_tests)
endif()
