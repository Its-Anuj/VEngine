add_library(VEngineCore Layers/LayerStack.cpp Application.cpp Window/Window.cpp Input/Input.cpp)

include_directories("Layers")
include_directories("Window")
include_directories("Events")
include_directories("Input")
include_directories("Profiling")
include_directories("Src/")

target_precompile_headers(VEngineCore PRIVATE VePCH.h)
