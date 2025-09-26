# VEngine

VEngine is my personal attempt at creating a small, modular **game engine** with modern **Vulkan 1.3** support.  
The project is split into a clean architecture with core and rendering layers, aiming to build up from simple rendering towards a more complete engine.

---

## Architecture

The engine is organized into the following main modules:

- **VEngineCore**
  - Handles **windowing** (via GLFW)
  - **Main application loop**
  - **Input** and **events**

- **VEngineRenderApi**
  - Abstract rendering interface
  - Responsible for **resources** and **rendering**
  - Current implementation: **VEngineModernVulkan**

- **VEngineModernVulkan**
  - Backend built on **Vulkan 1.3**
  - Uses **VMA** (Vulkan Memory Allocator) for memory management
  - Uses **Shaderc** for runtime shader compilation
  - Currently supports rendering a **textured quad**
  - Next steps:
    - Add **depth buffer support**
    - **Refactor and clean up** the architecture for clarity

---

## Dependencies

The engine relies on the following external libraries:

- [GLFW](https://www.glfw.org/) – Windowing & input  
- [GLM](https://github.com/g-truc/glm) – Math library  
- [stb](https://github.com/nothings/stb) – Image loading (textures)  
- [Vulkan 1.3 SDK](https://vulkan.lunarg.com/sdk/home) – Graphics API  
- [Shaderc](https://github.com/google/shaderc) – Shader compilation  
- [Vulkan Memory Allocator (VMA)](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator) – Memory management  

---

## Current Progress

✅ Rendering a **quad** with a texture.  
⬜ Next milestone: **Depth buffer support**.  
⬜ Planned: Refactoring the architecture for readability and modularity.

---

## Screenshot

*(Insert screenshot of the textured quad here)*
![alt text](<Screenshot 2025-09-26 210506.png>)
---

## Building

We use **CMake** + **Ninja** as the build system.

```bash
# Create build output folder
mkdir out

# Configure project (Debug build)
cmake -S . -B out -DCMAKE_BUILD_TYPE=Debug

# Compile shaders
call shadercompile

# Build with Ninja
cmake --build out -- -j
