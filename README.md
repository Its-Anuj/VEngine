# VEngine

VEngine is a modular C++ game engine project using [GLFW](VEngine/Libs/glfw/README.md) for windowing and input, and supports Vulkan rendering.

## Project Structure

```
.
├── CMakeLists.txt
├── LICENSE
├── main.cpp
├── README.md
├── .gitignore
├── .gitmodules
├── .vscode/
│   └── settings.json
└── VEngine/
    ├── CMakeLists.txt
    ├── Libs/
    │   └── glfw/
    │       ├── CMakeLists.txt
    │       ├── README.md
    │       ├── docs/
    │       ├── include/
    │       ├── src/
    │       └── ...
    └── Src/
        ├── Application.cpp/h
        ├── Log.h
        ├── Maths.h
        ├── VEngine.h
        ├── VePCH.h
        ├── Events/
        ├── Input/
        ├── Layers/
        ├── Profiling/
        ├── Rendering/
        └── Window/
```

## Building

This project uses [CMake](https://cmake.org/) for configuration and building.

### Prerequisites

- C++20 compatible compiler (MSVC, GCC, or Clang)
- [CMake](https://cmake.org/download/)
- Vulkan SDK (for Vulkan rendering)
- On Linux: development packages for X11 and/or Wayland if building GLFW from source

### Build Steps

1. **Configure the project:**

   ```sh
   cmake -S . -B build
   ```

2. **Build:**

   ```sh
   cmake --build build
   ```

3. **Run:**

   The main executable will be in `build/bin/Editor.exe` (or equivalent for your platform).

### Notes

- GLFW is included as a subdirectory in `VEngine/Libs/glfw`.
- Vulkan SDK path is set in the top-level `CMakeLists.txt`. Adjust as needed.
- Output directories for libraries and executables are set in `CMakeLists.txt`.

## Usage

- The engine core is in [`VEngine/Src`](VEngine/Src).
- The entry point is [`main.cpp`](main.cpp).
- Logging macros are defined in [`VEngine/Src/Log.h`](VEngine/Src/Log.h).
- Windowing and input are handled via GLFW.

## License

See [`LICENSE`](LICENSE) for details.

## Contributing

Pull requests and issues are welcome. See [`VEngine/Libs/glfw/README.md`](VEngine/Libs/glfw/README.md) for GLFW-specific contribution guidelines.

## References

- [GLFW Documentation](https://www.glfw.org/docs/latest/)
- [Vulkan SDK](https://vulkan.lunarg.com/)
