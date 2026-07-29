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

	// RAD-28 verification scaffolding: a 45°-rotated static body — its collider
	// debug-outline must align with the sprite (the pre-RAD-28 shape rotated
	// twice). Spawned at runtime on both load paths; running the game never
	// writes assets, so this never pollutes the authored level.
	{
		Entity ramp = m_Level->CreateEntity("RotatedColliderTest");
		ramp.AddComponent<SpriteComponent>(glm::vec4{ 1.0f, 0.6f, 0.1f, 1.0f });
		auto& rampTransform = ramp.GetComponent<TransformComponent>();
		rampTransform.Translation = { -4.0f, -2.0f, 0.0f };
		rampTransform.Rotation.z = glm::radians(45.0f);
		ramp.AddComponent<RigidBody2DComponent>(RigidBody2DComponent::BodyType::Static);
		ramp.AddComponent<BoxCollider2DComponent>();
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
	// Deliberately no SaveLevel/SaveAssetRegistry here: running the game must never
	// mutate authored content — saving is an explicit editor/tool action (RAD-17).
	// Asset clearing lives in GameplayState::OnExit, not here: OnDetach runs deferred
	// at end of frame, after the next state's OnEnter has already loaded its assets.
	m_Framebuffer.Reset();
	m_Level.Reset();
}

void GameLayer::OnFixedUpdate(Timestep ts)
{
	m_Level->OnFixedUpdate(ts);
}

void GameLayer::OnUpdate(Timestep ts)
{
	m_Framebuffer->Bind();

	RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1 });
	RenderCommand::Clear();

	m_Level->OnRender(Time::GetAlpha());

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

	// Add text dynamically
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.2f, 1.0f)); // Yellowish text color
	ImGui::SetCursorPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.4f, ImGui::GetIO().DisplaySize.y * 0.8f)); // Position at 40% width, 80% height
	ImGui::Text("Press F to use");
	ImGui::PopStyleColor();

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

	// RAD-28 verification cheats — the explicit physics verbs' first customers

	if (e.GetKeyCode() == Key::T)
	{
		// Teleport the Reaper into mid-air: proves the full verb chain — ECS
		// write, snapshot reset (no render smear), physics push, and
		// wake-from-sleep (a settled body must resume falling, not hang)
		Entity reaper = m_Level->FindEntityByName("Reaper");
		if (reaper)
			reaper.Teleport({ 4.0f, 2.0f, 0.0f });
		else
			GAME_WARN("Teleport cheat: no 'Reaper' entity in the level");
		return true;
	}

	if (e.GetKeyCode() == Key::G)
	{
		// Grow the green square's collider, then re-apply it: proves shapes
		// change ONLY on explicit refresh — the debug outline follows the
		// component instantly, the body changes when RefreshCollider runs
		Entity square = m_Level->FindEntityByName("Green Square");
		auto* bc2d = square ? square.TryGetComponent<BoxCollider2DComponent>() : nullptr;
		if (bc2d)
		{
			bc2d->Size *= 1.25f;
			if (bc2d->Size.x > 1.5f)
				bc2d->Size = { 0.5f, 0.5f };
			m_Level->RefreshCollider(square);
		}
		else
		{
			GAME_WARN("Collider cheat: no 'Green Square' entity with a collider in the level");
		}
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

	// The engine's asset paths warn-and-return-null on broken content — honor
	// the same contract here: recover to the debug level instead of crashing
	// on the first OnFixedUpdate
	if (!m_Level)
	{
		GAME_WARN("GameLayer: level asset '{}' failed to load - falling back to the debug level", ReaperContext::GetLevelAssetPath());
		m_Level = Ref<Level>::Create();
		CreateDEBUG();
		return;
	}

	m_Level->FindEntityByName("Camera").AddOrReplaceComponent<NativeScriptComponent>().Bind<CameraController>();
}

