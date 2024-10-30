#include "ReaperContext.h"

ReaperContext::ReaperContext()
{
	RADIANT_ASSERT(!s_Instance, "ReaperContext already exists!");
	s_Instance = this;

	// Initialize Systems
	AssetManager::Init();

	SetGameTheme();
}

ReaperContext::~ReaperContext()
{
	s_Instance = nullptr;
}

void ReaperContext::SetGameTheme()
{
	ImGuiStyle& style = ImGui::GetStyle();
	auto& colors = style.Colors;

	// Roundness settings
	style.WindowRounding = 5.25f;
	style.FrameRounding = 5.0f;
	style.ScrollbarRounding = 2.0f;
	style.GrabRounding = 2.0f;

	// Text colors
	colors[ImGuiCol_Text] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.36f, 0.42f, 0.47f, 1.00f);

	// Background colors
	colors[ImGuiCol_WindowBg] = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.15f, 0.16f, 0.17f, 1.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.11f, 0.12f, 0.13f, 0.94f);

	// Border colors
	colors[ImGuiCol_Border] = ImVec4(0.29f, 0.29f, 0.30f, 0.71f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

	// Frame colors
	colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.21f, 0.22f, 1.00f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.25f, 0.26f, 0.27f, 0.68f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.25f, 0.26f, 0.27f, 1.00f);

	// Title colors
	colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.11f, 0.12f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.10f, 0.10f, 0.10f, 0.75f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.14f, 0.14f, 0.15f, 1.00f);

	// Menu colors
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.15f, 1.00f);

	// Scrollbar colors
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.14f, 0.14f, 0.15f, 1.00f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.24f, 0.24f, 0.25f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.26f, 0.27f, 0.28f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.26f, 0.27f, 0.28f, 1.00f);

	// Check mark and slider colors
	colors[ImGuiCol_CheckMark] = ImVec4(0.35f, 0.75f, 0.95f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.24f, 0.54f, 0.67f, 1.00f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.60f, 0.78f, 1.00f);

	// Button colors
	colors[ImGuiCol_Button] = ImVec4(0.15f, 0.16f, 0.17f, 1.00f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.24f, 0.26f, 0.28f, 1.00f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.26f, 0.27f, 0.28f, 1.00f);

	// Header colors
	colors[ImGuiCol_Header] = ImVec4(0.14f, 0.15f, 0.16f, 1.00f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.23f, 0.24f, 0.26f, 1.00f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.23f, 0.24f, 0.26f, 1.00f);

	// Resize grip colors
	colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.27f, 0.28f, 1.00f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.28f, 0.30f, 1.00f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(0.29f, 0.31f, 0.34f, 1.00f);

	// Plot colors
	colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.62f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.64f, 0.66f, 0.68f, 1.00f);
	colors[ImGuiCol_PlotHistogram] = ImVec4(0.35f, 0.75f, 0.95f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.35f, 0.75f, 0.95f, 1.00f);

	// Selection colors
	colors[ImGuiCol_TextSelectedBg] = ImVec4(0.24f, 0.54f, 0.67f, 1.00f);
}
