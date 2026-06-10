add_library(osrssim_engine
  src/world.cpp
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

if(BUILD_TESTING)
  add_executable(osrssim_engine_tests
    tests/world_test.cpp
  )

  target_link_libraries(osrssim_engine_tests
    PRIVATE
      osrssim::engine
  )

  add_test(NAME osrssim_engine_tests COMMAND osrssim_engine_tests)
endif()
