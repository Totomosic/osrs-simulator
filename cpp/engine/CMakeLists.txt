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

if(EMSCRIPTEN)
  add_executable(osrssim_engine_wasm
    src/EngineBindings.cpp
  )

  target_link_libraries(osrssim_engine_wasm
    PRIVATE
      osrssim::engine
  )

  set_target_properties(osrssim_engine_wasm
    PROPERTIES
      OUTPUT_NAME EngineModule
      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/web/src/wasm/generated
      SUFFIX ".js"
  )

  target_link_options(osrssim_engine_wasm
    PRIVATE
      --bind
      -sALLOW_MEMORY_GROWTH=1
      -sENVIRONMENT=web
      -sEXPORT_ES6=1
      -sEXPORT_NAME=createGeneratedEngineModule
      -sMODULARIZE=1
  )
endif()

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

  add_executable(osrssim_engine_engine_test
    tests/EngineTest.cpp
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

  target_link_libraries(osrssim_engine_engine_test
    PRIVATE
      osrssim::engine
  )

  add_test(NAME osrssim_engine_scene_test COMMAND osrssim_engine_scene_test)
  add_test(NAME osrssim_engine_pathing_test COMMAND osrssim_engine_pathing_test)
  add_test(NAME osrssim_engine_world_test COMMAND osrssim_engine_world_test)
  add_test(NAME osrssim_engine_engine_test COMMAND osrssim_engine_engine_test)
endif()
