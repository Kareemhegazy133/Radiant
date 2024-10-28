#include "GameLayer.h"

using namespace Radiant;

GameLayer::GameLayer()
	: Layer("GameLayer")
{
	GAME_TRACE("GameLayer Constructor");

	RADIANT_ASSERT(!s_Instance, "GameLayer already exists!");
	s_Instance = this;

	SetGameTheme();

	AssetManager::Init();
}

GameLayer::~GameLayer()
{
	GAME_TRACE("GameLayer Destructor");

	delete s_Instance;
	s_Instance = nullptr;
}

void GameLayer::OnAttach()
{
	RADIANT_PROFILE_FUNCTION();

	PushState(m_MainMenuState);
}

void GameLayer::OnDetach()
{
	RADIANT_PROFILE_FUNCTION();

	m_MainMenuState.Reset();
	m_GamePausedState.Reset();
	m_GameplayState.Reset();

	if (m_NextState)
	{
		m_NextState.Reset();
	}
}

void GameLayer::OnUpdate(Timestep ts)
{
	if (m_StateStackSize > 0)
	{
		m_StateStack[m_StateStackSize - 1]->OnUpdate(ts);
	}
}

void GameLayer::OnImGuiRender()
{
	RADIANT_PROFILE_FUNCTION();

	if (m_StateStackSize > 0)
	{
		m_StateStack[m_StateStackSize - 1]->OnRender();
	}
}

void GameLayer::OnEvent(Event& e)
{
	RADIANT_PROFILE_FUNCTION();

	if (m_StateStackSize > 0)
	{
		m_StateStack[m_StateStackSize - 1]->OnEvent(e);
	}
}

void GameLayer::PopState()
{
	RADIANT_PROFILE_FUNCTION();

	if (m_StateStackSize > 0)
	{
		m_StateStack[m_StateStackSize - 1]->OnExit();
		m_StateStack[--m_StateStackSize].Reset();
	}
}

void GameLayer::SetGameTheme()
{
	// Set Font
	ImGuiIO& io = ImGui::GetIO();

	// Clear any previously loaded fonts
	io.Fonts->Clear();

	io.Fonts->AddFontFromFileTTF("Assets/Fonts/Euljiro.ttf", 18.0f);

	// Build the atlas
	unsigned char* tex_pixels = nullptr;
	int tex_width, tex_height;
	io.Fonts->GetTexDataAsRGBA32(&tex_pixels, &tex_width, &tex_height);

	// Set Theme
	auto& style = ImGui::GetStyle();
	auto& colors = ImGui::GetStyle().Colors;

	// Roundness settings
	style.WindowRounding = 5.3f;
	style.FrameRounding = 2.3f;
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
