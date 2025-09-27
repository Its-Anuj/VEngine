# VEngine

VEngine is a **personal, modular game engine** built with modern **Vulkan 1.3**.  
The engine is designed with a clean architecture, separating **core functionality** from **rendering**, with a focus on building from simple rendering toward a complete engine.

---

## Architecture

The engine is organized into the following main modules:

- **VEngineCore**  
  - Handles **windowing** via GLFW  
  - Provides the **main application loop**  
  - Manages **input** and **event handling**

- **VEngineRenderApi**  
  - Abstract rendering interface  
  - Manages **resources**, **pipelines**, and **rendering operations**  
  - Current implementation: **VulkanBackend** (refactored from VEngineModernVulkan)

- **VulkanBackend**  
  - Modern Vulkan 1.3 backend  
  - Integrates **depth buffer support**  
  - Uses **VMA** (Vulkan Memory Allocator) for GPU memory  
  - Uses **Shaderc** for runtime shader compilation  
  - Supports rendering **textured quads** with proper depth testing  
  - Refactored for **modularity and readability**  

---

## Dependencies

The engine relies on the following libraries:

- [GLFW](https://www.glfw.org/) – Windowing & input  
- [GLM](https://github.com/g-truc/glm) – Math library  
- [stb](https://github.com/nothings/stb) – Image loading (textures)  
- [Vulkan 1.3 SDK](https://vulkan.lunarg.com/sdk/home) – Graphics API  
- [Shaderc](https://github.com/google/shaderc) – Runtime shader compilation  
- [Vulkan Memory Allocator (VMA)](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator) – Memory management  

---

## Current Progress

✅ Render **textured quad** with **depth buffer support**  
✅ Depth testing implemented for proper occlusion  
⬜ Next: Refactor Vulkan backend fully into a clean and modular structure  

---

## Screenshot

Here’s a current screenshot showing a **textured quad with depth buffer enabled**:  

![Current](<Screenshot 2025-09-27 130808.png>)
---

## Build Instructions

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
