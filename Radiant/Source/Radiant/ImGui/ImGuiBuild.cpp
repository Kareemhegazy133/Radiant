#include "Radiant/rdpch.h"

// The vendored ImGui library target builds only the core; the platform/renderer
// backends ship as sources and are compiled into the engine here, in a single
// translation unit, with the loader pinned to glad.
#define IMGUI_IMPL_OPENGL_LOADER_GLAD
#include <backends/imgui_impl_opengl3.cpp>
#include <backends/imgui_impl_glfw.cpp>
