cmake_minimum_required(VERSION 3.30)
project(NumericalMethods
  LANGUAGES CXX C
)


# =========================
#         CONFIG
# =========================

add_library(Config INTERFACE)

if(WIN32)
  target_compile_definitions(Config INTERFACE PLATFORM_WINDOWS)
elseif(APPLE)
  target_compile_definitions(Config INTERFACE PLATFORM_MAC)
elseif(UNIX)
  target_compile_definitions(Config INTERFACE PLATFORM_LINUX)
endif()

# ==================================
#         SET C++ STANDARD
# ==================================

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ==================================
#    CONFIGURE COMPILER WARNINGS
# ==================================

add_library(Warnings INTERFACE)

if(MSVC)
  target_compile_options(Warnings INTERFACE
        /W4
        /WX
    )
else()
  target_compile_options(Warnings INTERFACE
        -Wall
        -Wextra
        -Wpedantic
        -Werror
        -Wshadow
        -Wnon-virtual-dtor
        -Woverloaded-virtual
        -Wnull-dereference
        -Wimplicit-fallthrough
        -Wcast-align=strict
        -Wformat=2
    )
endif()

# ==================================
# INTERPROCEDURAL OPTIMIZATION (LTO)
# ==================================

# If compiled with LTO, make sure its supported by the compiler.
if(CMAKE_INTERPROCEDURAL_OPTIMIZATION)
  include(CheckIPOSupported)
  check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)
  if(ipo_supported)
    message(STATUS "IPO / LTO enabled!")
  else()
    message(WARNING "IPO is not supported: ${ipo_error}. Falling back to standard build.")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)
  endif()
endif()

# ==================================
#        FETCH DEPENDENCIES
# ==================================

include(FetchContent)

set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # don't build the supplied examples
set(BUILD_GAMES    OFF CACHE BOOL "" FORCE) # don't build the supplied example games
set(GLFW_BUILD_WAYLAND ON CACHE BOOL "" FORCE) # enable wayland on linux
set(GLFW_BUILD_X11 OFF CACHE BOOL "" FORCE) # disable x11 on linux (switch these settings around as you please)

FetchContent_Declare(
    raylib
    GIT_REPOSITORY "https://github.com/raysan5/raylib.git"
    GIT_TAG 6.0
)

FetchContent_MakeAvailable(raylib)

# ==================================
#            ADD CODE
# ==================================

add_subdirectory(Code)
