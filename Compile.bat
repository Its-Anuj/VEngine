@echo off
REM -----------------------------
REM Compile.bat for Windows with build type selection and timestamps
REM -----------------------------

setlocal enabledelayedexpansion

REM 1. Ask for build type
set /p "build_type=Enter build type (Debug or Release): "
if "%build_type%"=="" set build_type=Debug
if /i not "%build_type%"=="Debug" if /i not "%build_type%"=="Release" (
    echo Invalid build type. Defaulting to Debug.
    set build_type=Debug
)

echo [%TIME%] Starting %build_type% build process...

REM 2. Set paths based on build type
set "BUILD_DIR=out/build/%build_type%"
set "BIN_DIR=out/%build_type%/bin"

REM 3. Create folders if they don't exist
if not exist out mkdir out
if not exist out/build mkdir out/build
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
if not exist out/%build_type% mkdir out/%build_type%
if not exist "%BIN_DIR%" mkdir "%BIN_DIR%"
echo [%TIME%] Folders checked/created.

REM 4. Configure and generate build files with CMake
set "start_time=%TIME%"
echo [%start_time%] Running CMake configuration...
cmake -S . -B "%BUILD_DIR%" -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=%build_type%
set "end_time=%TIME%"
echo [%end_time%] CMake configuration complete.
call :CalculateTime "%start_time%" "%end_time%" "CMake configuration"

REM 5. Build the project
set "start_time=%TIME%"
echo [%start_time%] Building project...
cmake --build "%BUILD_DIR%" --config %build_type%
set "end_time=%TIME%"
echo [%end_time%] Build complete.
call :CalculateTime "%start_time%" "%end_time%" "Build"

REM 6. Compile shaders
set "start_time=%TIME%"
echo [%start_time%] Compiling shaders...

REM Compile vertex shader
echo Compiling VertexShader.vert...
"C:/VulkanSDK/1.3.275.0/Bin/glslc.exe" VertexShader.vert -o "%BIN_DIR%/vert.spv"
if !errorlevel! neq 0 (
    echo ERROR: Failed to compile vertex shader!
    pause
    exit /b 1
)

REM Compile fragment shader
echo Compiling FragmentShader.frag...
"C:/VulkanSDK/1.3.275.0/Bin/glslc.exe" FragmentShader.frag -o "%BIN_DIR%/frag.spv"
if !errorlevel! neq 0 (
    echo ERROR: Failed to compile fragment shader!
    pause
    exit /b 1
)

set "end_time=%TIME%"
echo [%end_time%] Shaders compiled.
call :CalculateTime "%start_time%" "%end_time%" "Shader compilation"

echo [%TIME%] All steps finished!
echo.
echo Build output: %BUILD_DIR%
echo Binary output: %BIN_DIR%
echo.
pause
exit /b 0

REM -----------------------------
REM Function to calculate time difference
REM -----------------------------
:CalculateTime
setlocal
set "start=%~1"
set "end=%~2"
set "step_name=%~3"

REM Convert times to seconds
for /f "tokens=1-3 delims=:., " %%a in ("%start%") do (
    set /a "start_secs=(((%%a * 60) + %%b) * 60) + %%c"
)
for /f "tokens=1-3 delims=:., " %%a in ("%end%") do (
    set /a "end_secs=(((%%a * 60) + %%b) * 60) + %%c"
)

set /a "duration_secs=end_secs - start_secs"
set /a "hours=duration_secs / 3600"
set /a "minutes=(duration_secs %% 3600) / 60"
set /a "seconds=duration_secs %% 60"

if %hours% gtr 0 (
    echo   %step_name% took: %hours%h %minutes%m %seconds%s
) else if %minutes% gtr 0 (
    echo   %step_name% took: %minutes%m %seconds%s
) else (
    echo   %step_name% took: %seconds%s
)

endlocal
goto :eof