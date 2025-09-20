@echo off
REM -----------------------------
REM Compile.bat for Windows with timestamps
REM -----------------------------

echo [%TIME%] Starting build process...

REM 1. Create folders if they don't exist
if not exist out mkdir out
if not exist out/build mkdir out/build
if not exist out/build/Debug mkdir out/build/Debug
echo [%TIME%] Folders checked/created.

REM 2. Configure and generate build files with CMake
echo [%TIME%] Running CMake configuration...
cmake -S . -B out/build/Debug -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
echo [%TIME%] CMake configuration complete.

REM 3. Build the project
echo [%TIME%] Building project...
cmake --build out/build/Debug --config Debug
echo [%TIME%] Build complete.

REM 4. Compile shaders
echo [%TIME%] Compiling shaders...
call ShaderCompile.bat
echo [%TIME%] Shaders compiled.

echo [%TIME%] All steps finished!
pause
