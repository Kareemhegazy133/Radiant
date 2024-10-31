#include "GameLayer.h"

#include "Core/ReaperContext.h"
#include "Core/GameStateManager.h"

#include "CameraController.h"

GameLayer::GameLayer()
	: Layer("GameLayer")
{
	GAME_TRACE("GameLayer Constructor");
}

GameLayer::~GameLayer()
{
	GAME_TRACE("GameLayer Destructor");

}

void GameLayer::OnAttach()
{
	if (!AssetManager::LoadAssetRegistry(ReaperContext::GetGameplayAssetRegistryPath()))
	{
		m_Level = Ref<Level>::Create();
		CreateDEBUG();
	}
	else
	{
		LoadDEBUG();
	}

	FramebufferSpecification fbSpec;
	fbSpec.Attachments = { FramebufferTextureFormat::RGBA8, FramebufferTextureFormat::RED_INTEGER, FramebufferTextureFormat::Depth };
	GameApplication& game = GameApplication::Get();
	fbSpec.Width = game.GetWindow().GetWidth();
	fbSpec.Height = game.GetWindow().GetHeight();
	m_Framebuffer = Framebuffer::Create(fbSpec);

}

void GameLayer::OnDetach()
{
	AssetManager::SaveLevel(m_Level, ReaperContext::GetLevelAssetPath());
	AssetManager::SaveAssetRegistry(ReaperContext::GetGameplayAssetRegistryPath());
	AssetManager::ClearAssets();

	m_Framebuffer.Reset();
	m_Level.Reset();
}

void GameLayer::OnUpdate(Timestep ts)
{
	m_Framebuffer->Bind();

	RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1 });
	RenderCommand::Clear();

	m_Level->OnUpdate(ts);
	m_Level->OnRender();

	m_Framebuffer->Unbind();
}

void GameLayer::OnImGuiRender()
{
	// Disable window decorations and force ImGui window to take the size of the game texture
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
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
	ImGui::Image(textureID, ImGui::GetIO().DisplaySize, ImVec2(0, 1), ImVec2(1, 0));

	ImGui::End();

	ImGui::PopStyleVar();
}

void GameLayer::OnEvent(Event& e)
{
	EventDispatcher dispatcher(e);
	dispatcher.Dispatch<WindowResizeEvent>(RADIANT_BIND_EVENT_FN(GameLayer::OnWindowResized));
	dispatcher.Dispatch<KeyPressedEvent>(RADIANT_BIND_EVENT_FN(GameLayer::OnKeyPressed));
}

bool GameLayer::OnWindowResized(WindowResizeEvent& e)
{
	RADIANT_PROFILE_FUNCTION();

	m_Level->OnViewportResize(e.GetWidth(), e.GetHeight());

	return false;
}

bool GameLayer::OnKeyPressed(KeyPressedEvent& e)
{
	if (e.GetKeyCode() == Key::Escape)
	{
		GameStateManager::Get()->PushState<GamePausedState>();
		return true;
	}
	return false;
}

void GameLayer::CreateDEBUG()
{
	auto square = m_Level->CreateEntity("Green Square");
	square.AddComponent<SpriteComponent>(glm::vec4{ 0.0f, 1.0f, 0.0f, 1.0f });
	square.AddComponent<RigidBody2DComponent>(RigidBody2DComponent::BodyType::Dynamic);
	square.AddComponent<BoxCollider2DComponent>();

	//AssetManager::LoadAsset("Assets/Textures/Checkerboard.png");

	auto reaper = m_Level->CreateEntity("Reaper");
	reaper.AddComponent<SpriteComponent>(AssetManager::LoadAsset("Assets/Textures/ReaperIcon.jpg")->Handle);
	reaper.GetComponent<TransformComponent>().Translation = { 4.0f, 0.0f, 0.0f };
	reaper.AddComponent<RigidBody2DComponent>(RigidBody2DComponent::BodyType::Dynamic);
	reaper.AddComponent<BoxCollider2DComponent>();

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

void GameLayer::LoadDEBUG()
{
	m_Level = AssetManager::LoadAsset<Level>(ReaperContext::GetLevelAssetPath());
	m_Level->FindEntityByName("Camera").AddOrReplaceComponent<NativeScriptComponent>().Bind<CameraController>();
}

