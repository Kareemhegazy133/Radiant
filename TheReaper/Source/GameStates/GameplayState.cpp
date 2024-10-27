#include "GameplayState.h"

#include <imgui/imgui.h>

#include "Layers/GameLayer.h"

#include "CameraController.h"

void GameplayState::OnEnter()
{
	GAME_INFO("Gameplay OnEnter");

	FramebufferSpecification fbSpec;
	fbSpec.Attachments = { FramebufferTextureFormat::RGBA8, FramebufferTextureFormat::RED_INTEGER, FramebufferTextureFormat::Depth };
	GameApplication& game = GameApplication::Get();
	fbSpec.Width = game.GetWindow().GetWidth();
	fbSpec.Height = game.GetWindow().GetHeight();
	m_Framebuffer = Framebuffer::Create(fbSpec);

	if (!AssetManager::LoadAssetRegistry(m_AssetRegistryPath))
	{
		m_Level = CreateRef<Level>();

		CreateDEBUG();
	}
	else
	{
		LoadDEBUG();
	}

}

void GameplayState::OnExit()
{
	GAME_INFO("Gameplay OnExit");

	// Save everything if transitioning to MainMenuState
	if (GameLayer::Get()->GetNextState() == GameLayer::Get()->GetMainMenuState())
	{
		AssetManager::SaveLevel(m_Level, "Assets/Levels/Level.rdlvl");
		AssetManager::SaveAssetRegistry(m_AssetRegistryPath);
		AssetManager::ClearAssetRegistry();
		m_Level.reset();
	}
}

void GameplayState::OnUpdate(Timestep ts)
{
	RADIANT_PROFILE_SCOPE("GameplayState Renderer Prep");

	m_Framebuffer->Bind();

	RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1 });
	RenderCommand::Clear();

	m_Level->OnUpdate(ts);
	m_Level->OnRender();

	m_Framebuffer->Unbind();
}

void GameplayState::OnRender()
{
	// Disable window decorations and force ImGui window to take the size of the game texture
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);

	// Temporarily remove window padding
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

	ImGui::Begin(
		"Gameplay",
		nullptr,
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoScrollWithMouse
	);

	uint64_t textureID = m_Framebuffer->GetColorAttachmentRendererID();

	// Display the game texture using ImGui::Image
	ImGui::Image(textureID, ImGui::GetIO().DisplaySize, ImVec2(0, 1), ImVec2(1, 0));

	ImGui::End();

	// Restore previous padding style
	ImGui::PopStyleVar();
}

void GameplayState::OnEvent(Event& e)
{
	EventDispatcher dispatcher(e);
	dispatcher.Dispatch<WindowResizeEvent>(RADIANT_BIND_EVENT_FN(GameplayState::OnWindowResized));
	dispatcher.Dispatch<KeyPressedEvent>(RADIANT_BIND_EVENT_FN(GameplayState::OnKeyPressed));
}

bool GameplayState::OnWindowResized(WindowResizeEvent& e)
{
	RADIANT_PROFILE_FUNCTION();

	m_Level->OnViewportResize(e.GetWidth(), e.GetHeight());

	return false;
}

bool GameplayState::OnKeyPressed(KeyPressedEvent& e)
{
	if (e.GetKeyCode() == Key::Escape)
	{
		//GameLayer::Get()->PushState(GameLayer::Get()->GetGamePausedState());
	}

	// TEMP
	if (e.GetKeyCode() == Key::X)
	{
		GameLayer::Get()->ChangeState(GameLayer::Get()->GetMainMenuState());
	}
	return true;
}

void GameplayState::CreateDEBUG()
{
	AssetManager::LoadAsset<Font>("Assets/Fonts/OpenSans/OpenSans-Regular.ttf");

	auto square = m_Level->CreateEntity("Green Square");
	square.AddComponent<SpriteComponent>(glm::vec4{ 0.0f, 1.0f, 0.0f, 1.0f });
	square.AddComponent<RigidBody2DComponent>(RigidBody2DComponent::BodyType::Dynamic);
	square.AddComponent<BoxCollider2DComponent>();

	//AssetManager::LoadAsset("Assets/Textures/Checkerboard.png");

	auto checkerboard = m_Level->CreateEntity("Checkerboard");
	checkerboard.AddComponent<SpriteComponent>(AssetManager::LoadAsset("Assets/Textures/Checkerboard.png")->Handle);
	checkerboard.GetComponent<TransformComponent>().Translation = { 4.0f, 0.0f, 0.0f };
	checkerboard.AddComponent<RigidBody2DComponent>(RigidBody2DComponent::BodyType::Dynamic);
	checkerboard.AddComponent<BoxCollider2DComponent>();

	auto text = m_Level->CreateEntity("Text");
	text.AddComponent<TextComponent>("Hello World!", 24.f);

	//m_CheckerboardTexture = Texture2D::Create("Assets/Textures/Checkerboard.png");
	//m_SpriteSheet = Texture2D::Create("Assets/SpriteSheets/RPGpack_sheet_2X.png");

	//m_TextureStairs = SubTexture2D::CreateFromCoords(m_SpriteSheet, { 7, 6 }, { 128, 128 });
	//m_TextureBarrel = SubTexture2D::CreateFromCoords(m_SpriteSheet, { 8, 2 }, { 128, 128 });
	//m_TextureTree = SubTexture2D::CreateFromCoords(m_SpriteSheet, { 2, 1 }, { 128, 128 }, {1, 2});

	auto platform = m_Level->CreateEntity("Platform");
	platform.AddComponent<SpriteComponent>(glm::vec4{ 0.0f, 1.0f, 1.0f, 1.0f });
	platform.GetComponent<TransformComponent>().Translation = { 0.0f, -4.0f, 0.0f };
	platform.GetComponent<TransformComponent>().Scale = { 15.0f, 1.0f, 0.0f };
	platform.AddComponent<RigidBody2DComponent>(RigidBody2DComponent::BodyType::Static);
	platform.AddComponent<BoxCollider2DComponent>();

	// Setup Camera
	m_Camera = m_Level->CreateEntity("Camera");
	m_Camera.AddComponent<CameraComponent>().Primary = true;
	m_Camera.GetComponent<CameraComponent>().Camera.SetViewportSize(1280, 720);
	m_Camera.AddComponent<NativeScriptComponent>().Bind<CameraController>();
}

void GameplayState::LoadDEBUG()
{
	AssetManager::LoadAsset<Font>("Assets/Fonts/OpenSans/OpenSans-Regular.ttf");
	m_Level = AssetManager::LoadAsset<Level>("Assets/Levels/Level.rdlvl");

	m_Level->FindEntityByName("Camera").AddOrReplaceComponent<NativeScriptComponent>().Bind<CameraController>();
	//AssetManager::LoadAssetPack("Assets/AssetPack.rdap");
}

