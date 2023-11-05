cmake_minimum_required(VERSION 3.12)
project(WasmFluidSimulation)

if("${CMAKE_SYSTEM}" MATCHES "Linux" OR "${CMAKE_SYSTEM}" MATCHES "Darwin-*")

    find_package(OpenGL REQUIRED)

    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    add_subdirectory(external/glfw-3.3.8)
endif()

include_directories(external/glfw-3.3.8/include)
set(CMAKE_CXX_STANDARD 17)

if (NOT DEFINED EMSCRIPTEN)

    add_executable(WasmFluidSimulation main.cpp external/glad.c Grid2D.h Grid2D.cpp
            external/glad.h)
else()
    add_executable(WasmFluidSimulation main.cpp Grid2D.h Grid2D.cpp)
endif (NOT DEFINED EMSCRIPTEN)

    target_link_libraries(WasmFluidSimulation glfw dl)

if (DEFINED EMSCRIPTEN)
    set_target_properties(WasmFluidSimulation
            PROPERTIES SUFFIX ".html"
            LINK_FLAGS " -O3 --bind -s USE_GLFW=3 -s WASM=1 -gsource-map ")
endif (DEFINED EMSCRIPTEN)