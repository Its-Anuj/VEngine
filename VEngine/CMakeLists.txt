add_library(VEngine Src/Layers/LayerStack.cpp Src/UUID/UUID.cpp Src/Application.cpp Src/Window/Window.cpp Src/Rendering/Renderer.cpp Src/Input/Input.cpp)

add_subdirectory(Libs/glfw)
add_subdirectory(Src/Rendering)

# 1. Add the subdirectory containing VMA
add_subdirectory(Libs/VMA)

# Add glm subdir
add_subdirectory(Libs/glm)
# Disable extras
set(SHADERC_SKIP_TESTS ON CACHE BOOL "Skip building shaderc tests" FORCE)
set(SHADERC_SKIP_EXAMPLES ON CACHE BOOL "Skip building shaderc examples" FORCE)
set(SHADERC_SKIP_EXECUTABLES ON CACHE BOOL "Skip building shaderc executables" FORCE)
set(SHADERC_ENABLE_WERROR_COMPILE OFF CACHE BOOL "" FORCE)

# Tell Shaderc where to find its dependencies
set(SHADERC_THIRD_PARTY_ROOT_DIR "${CMAKE_SOURCE_DIR}/VEngine/Libs/shaderc/third_party" CACHE PATH "Shaderc third_party root" FORCE)
set(SHADERC_SPIRV_TOOLS_DIR "${CMAKE_SOURCE_DIR}/VEngine/Libs/SPIRV-Tools" CACHE PATH "SPIRV-Tools source directory" FORCE)
set(SHADERC_GLSLANG_DIR "${CMAKE_SOURCE_DIR}/VEngine/Libs/glslang" CACHE PATH "glslang source directory" FORCE)

# Now add Shaderc
add_subdirectory(Libs/shaderc)

include_directories("Src/Layers")
include_directories("Src/UUID")
include_directories("Src/Window")
include_directories("Src/Events")
include_directories("Src/Input")
include_directories("Src/Profiling")
include_directories("Src/Rendering/")
include_directories("Src/Rendering/Vulkan")
include_directories("Src/")
include_directories("Libs/glfw/include/GLFW/")
include_directories("Libs/glm/glm/")
include_directories("Libs/VMA/include/")
include_directories("Libs/stb/")
include_directories("Libs/shaderc/libshaderc/include/")

include_directories("C:/VulkanSDK/1.3.275.0/Include/vulkan")
include_directories("C:/VulkanSDK/1.3.275.0/Include/")
include_directories("C:/VulkanSDK/1.3.275.0/Include/vk_video")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(VEngine PRIVATE VENGINE_DEBUG_BUILD)
    message("[ENGINE] Debug build enabled")
endif()
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(VEngine PRIVATE VENGINE_RELEASE_BUILD)
    message("[ENGINE] Release build enabled")
endif()
if (MINGW)
    message(MINGW)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
endif()
# Add precompiled header (for example "pch.h")
target_precompile_headers(VEngine PRIVATE Src/VePCH.h)