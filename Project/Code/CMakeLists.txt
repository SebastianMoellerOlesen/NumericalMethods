add_executable(Project)

set(SOURCES
  main.cpp
  Simulation.cpp
)

set(HEADERS
  Simulation.hpp
  SmoothingKernals.hpp
  Geometries.hpp
  Utils.hpp
)

target_sources(Project PUBLIC
  ${SOURCES}
  ${HEADERS}
)


if(WIN32)
  target_compile_definitions(Project PRIVATE PLATFORM_WINDOWS)
endif()

target_include_directories(Project PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if (WIN32)
  target_link_libraries(Project raylib ImGuiRaylib)
else()
  target_link_libraries(Project raylib TBB::tbb ImGuiRaylib)
endif()
