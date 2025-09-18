cmake_minimum_required(VERSION 3.10.0)
project(Editor VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
add_subdirectory(VEngine)

# Check if Libs exist

# Try to find a system GLFW3 first
# Check if we have a local glfw in libs/

include_directories("VEngine/Src/Layers")
include_directories("VEngine/Src/")
include_directories("VEngine/Src/Input")
include_directories("VEngine/Src/Rendering/")
include_directories("VEngine/Src/UUID/")
include_directories("VEngine/Src/Events")
include_directories("VEngine/Src/Window")
include_directories("VEngine/Libs/glfw/include/GLFW/")
include_directories("VEngine/Src/Profiling")
include_directories("VEngine/Src/Rendering/Vulkan")

# Set output directories for all targets
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)   # Static libraries (.lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)   # Shared libraries (.dll)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)   # Executables (.exe)

add_executable(Editor main.cpp)

set(APP_DIR ${CMAKE_SOURCE_DIR})
set(ENGINE_DIR ${SIMULATION_DIR}/VEngine)
set(ENGINE_SRC_DIR ${ENGINE_DIR}/Src)
set(ENGINE_LIBS_DIR ${ENGINE_DIR}/Libs)

target_link_libraries(Editor PRIVATE VEngine glfw VEngineVulkan)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(Editor PRIVATE VENGINE_DEBUG_BUILD)
    message("[ENGINE] Debug build enabled")
endif()
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(Simulation PRIVATE VENGINE_RELEASE_BUILD)
    message("[ENGINE] Release build enabled")
endif()
 
# System OpenGL
if(WIN32)
    target_link_libraries(Editor PRIVATE kernel32 user32 gdi32 comdlg32)
elseif(APPLE)
    target_link_libraries(Editor PRIVATE "-framework OpenGL")
else()
    target_link_libraries(Editor PRIVATE GL)
endif()

set(VULKAN_SDK "C:/VulkanSDK/1.3.275.0/")

target_link_libraries(Editor PRIVATE ${VULKAN_SDK}/Lib/vulkan-1.lib )
