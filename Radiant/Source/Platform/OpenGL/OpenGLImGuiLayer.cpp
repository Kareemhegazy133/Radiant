#include "rdpch.h"
#include "OpenGLImGuiLayer.h"

#include "Core/GameApplication.h"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include <GLFW/glfw3.h>
#include <glad/glad.h>

namespace Radiant {

	OpenGLImGuiLayer::OpenGLImGuiLayer()
	{
	}

	OpenGLImGuiLayer::OpenGLImGuiLayer(const std::string& name)
	{
	}

	OpenGLImGuiLayer::~OpenGLImGuiLayer()
	{
	}

	void OpenGLImGuiLayer::OnAttach()
	{
		RADIANT_PROFILE_FUNCTION();

		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
		//io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows

		// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		// Setup Dear ImGui style
		ImGui::StyleColorsDark();
		SetDarkThemeColors();

		GameApplication& game = GameApplication::Get();
		GLFWwindow* window = static_cast<GLFWwindow*>(game.GetWindow().GetNativeWindow());

		// Setup Platform/Renderer bindings
		ImGui_ImplGlfw_InitForOpenGL(window, true);
		ImGui_ImplOpenGL3_Init("#version 410");
	}

	void OpenGLImGuiLayer::OnDetach()
	{
		RADIANT_PROFILE_FUNCTION();

		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	void OpenGLImGuiLayer::Begin()
	{
		RADIANT_PROFILE_FUNCTION();

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}

	void OpenGLImGuiLayer::End()
	{
		RADIANT_PROFILE_FUNCTION();

		ImGuiIO& io = ImGui::GetIO();
		GameApplication& game = GameApplication::Get();
		io.DisplaySize = ImVec2((float)game.GetWindow().GetWidth(), (float)game.GetWindow().GetHeight());

		static bool demo = true;
		ImGui::ShowDemoWindow(&demo);

		// Rendering
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_current_context);
		}
	}

	void OpenGLImGuiLayer::OnImGuiRender()
	{
	}

}