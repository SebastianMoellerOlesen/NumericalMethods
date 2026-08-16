add_executable(Project)

set(SOURCES
  main.cpp
  Simulation.cpp
)

set(HEADERS
  Simulation.hpp
)

target_sources(Project PUBLIC
  ${SOURCES}
  ${HEADERS}
)

target_include_directories(Project PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Project PRIVATE raylib TBB::tbb ImGuiRaylib)
