add_library(osrssim_engine
  src/Engine.cpp
  src/Pathing.cpp
  src/Scene.cpp
  src/Tile.cpp
  src/World.cpp
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
  add_executable(osrssim_engine_scene_test
    tests/SceneTest.cpp
  )

  add_executable(osrssim_engine_pathing_test
    tests/PathingTest.cpp
  )

  add_executable(osrssim_engine_world_test
    tests/WorldTest.cpp
  )

  target_link_libraries(osrssim_engine_scene_test
    PRIVATE
      osrssim::engine
  )

  target_link_libraries(osrssim_engine_pathing_test
    PRIVATE
      osrssim::engine
  )

  target_link_libraries(osrssim_engine_world_test
    PRIVATE
      osrssim::engine
  )

  add_test(NAME osrssim_engine_scene_test COMMAND osrssim_engine_scene_test)
  add_test(NAME osrssim_engine_pathing_test COMMAND osrssim_engine_pathing_test)
  add_test(NAME osrssim_engine_world_test COMMAND osrssim_engine_world_test)
endif()
