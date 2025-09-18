# VEngine

VEngine is a modular C++ game engine project. Its architecture is divided into three main parts:

## Engine Structure

### 1. VEngineCore
Handles all core engine functionality:
- Windowing
- Input
- Events
- Logging
- Application lifecycle

### 2. VEngineRenderAPI
Responsible for rendering. It is further divided into:
- **VEngineVulkan:** Vulkan-based rendering backend
- **VEngineOpenGL:** OpenGL-based rendering backend

### 3. VEngineExtensions
Provides extended engine features:
- Asset loading
- Shader maps
- Cameras
- Other utilities

## Project Layout

```
VEngine/
├── Src/
│   ├── Core/           # VEngineCore: windowing, input, events, etc.
│   ├── Rendering/      # VEngineRenderAPI
│   │   ├── Vulkan/     # VEngineVulkan
│   │   └── OpenGL/     # VEngineOpenGL
│   └── Extensions/     # VEngineExtensions: assets, cameras, etc.
├── Libs/
│   └── glfw/           # Windowing/input library
```

## Building

This project uses [CMake](https://cmake.org/) for configuration and building.

### Prerequisites

- C++20 compatible compiler (MSVC, GCC, or Clang)
- [CMake](https://cmake.org/download/)
- Vulkan SDK (for Vulkan rendering)
- OpenGL development libraries (for OpenGL backend)

### Build Steps

```sh
cmake -S . -B build
cmake --build build
```

## Usage

- The engine core is in `VEngine/Src/Core`.
- Rendering backends are in `VEngine/Src/Rendering/Vulkan` and `VEngine/Src/Rendering/OpenGL`.
- Extensions and utilities are in `VEngine/Src/Extensions`.

## License

See [`LICENSE`](LICENSE) for details.

## References

- [GLFW Documentation](https://www.glfw.org/docs/latest/)
- [Vulkan SDK](https://vulkan.lunarg.com/)

also include:
libgcc_s_seh-1.dll
libstdc++-6.dll

if .dll errors comes 