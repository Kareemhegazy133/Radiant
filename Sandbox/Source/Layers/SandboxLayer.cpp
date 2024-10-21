#include "SandboxLayer.h"

#include "CameraController.h"

using namespace Radiant;

static Ref<Font> s_Font;

SandboxLayer::SandboxLayer()
	: Layer("SandboxLayer")
{
	GAME_TRACE("SandboxLayer Constructor");

	AssetManager::Init();
}

SandboxLayer::~SandboxLayer()
{
	GAME_TRACE("SandboxLayer Destructor");
}

void SandboxLayer::OnAttach()
{
	RADIANT_PROFILE_FUNCTION();

	s_Font = AssetManager::LoadAsset<Font>("Assets/Fonts/OpenSans/OpenSans-Regular.ttf");
	m_Level = AssetManager::LoadAsset<Level>("Assets/Levels/Level.rdlvl");
	//m_Level = CreateRef<Level>();

	//auto square = m_Level->CreateEntity("Green Square");
	//square.AddComponent<SpriteComponent>(glm::vec4{ 0.0f, 1.0f, 0.0f, 1.0f });
	//square.AddComponent<RigidBody2DComponent>(RigidBody2DComponent::BodyType::Dynamic);
	//square.AddComponent<BoxCollider2DComponent>();

	////AssetManager::LoadAsset("Assets/Textures/Checkerboard.png");

	//auto checkerboard = m_Level->CreateEntity("Checkerboard");
	//checkerboard.AddComponent<SpriteComponent>(AssetManager::LoadAsset("Assets/Textures/Checkerboard.png")->Handle);
	//checkerboard.GetComponent<TransformComponent>().Translation = { 4.0f, 0.0f, 0.0f };
	//checkerboard.AddComponent<RigidBody2DComponent>(RigidBody2DComponent::BodyType::Dynamic);
	//checkerboard.AddComponent<BoxCollider2DComponent>();

	//auto text = m_Level->CreateEntity("Text");
	//text.AddComponent<TextComponent>("Hello World!", 24.f);

	////m_CheckerboardTexture = Texture2D::Create("Assets/Textures/Checkerboard.png");
	////m_SpriteSheet = Texture2D::Create("Assets/SpriteSheets/RPGpack_sheet_2X.png");

	////m_TextureStairs = SubTexture2D::CreateFromCoords(m_SpriteSheet, { 7, 6 }, { 128, 128 });
	////m_TextureBarrel = SubTexture2D::CreateFromCoords(m_SpriteSheet, { 8, 2 }, { 128, 128 });
	////m_TextureTree = SubTexture2D::CreateFromCoords(m_SpriteSheet, { 2, 1 }, { 128, 128 }, {1, 2});

	//auto platform = m_Level->CreateEntity("Platform");
	//platform.AddComponent<SpriteComponent>(glm::vec4{ 0.0f, 1.0f, 1.0f, 1.0f });
	//platform.GetComponent<TransformComponent>().Translation = { 0.0f, -4.0f, 0.0f };
	//platform.GetComponent<TransformComponent>().Scale = { 15.0f, 1.0f, 0.0f };
	//platform.AddComponent<RigidBody2DComponent>(RigidBody2DComponent::BodyType::Static);
	//platform.AddComponent<BoxCollider2DComponent>();

	//// Setup Camera
	//m_Camera = m_Level->CreateEntity("Camera");
	//m_Camera.AddComponent<CameraComponent>().Primary = true;
	//m_Camera.GetComponent<CameraComponent>().Camera.SetViewportSize(1280, 720);
	//m_Camera.AddComponent<NativeScriptComponent>().Bind<CameraController>();

	m_Level->FindEntityByName("Camera").AddComponent<NativeScriptComponent>().Bind<CameraController>();

	//AssetManager::LoadAssetPack("Assets/AssetPack.rdap");
}

void SandboxLayer::OnDetach()
{
	RADIANT_PROFILE_FUNCTION();

	AssetManager::SaveLevel(m_Level, "Assets/Levels/Level.rdlvl");
	//AssetManager::CreateAssetPack("Assets/AssetPack.rdap");
}

void SandboxLayer::OnUpdate(Timestep ts)
{
	RADIANT_PROFILE_SCOPE("Renderer Prep");
	RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1 });
	RenderCommand::Clear();

	m_Level->OnUpdate(ts);
	m_Level->OnRender();

	//Renderer2D::DrawString("Hello", s_Font, glm::mat4(1.0f), {0.2f, 0.3f, 0.8f, 1.f});

	//{1
	//	static float rotation = 0.0f;
	//	rotation += ts * 50.0f;

	//	RADIANT_PROFILE_SCOPE("Renderer Draw");
	//	Renderer2D::BeginScene(m_CameraController.GetCamera());

	//	Renderer2D::DrawRotatedQuad({ 1.0f, 0.0f }, { 0.8f, 0.8f }, glm::radians(-45.0f), { 0.8f, 0.2f, 0.3f, 1.0f });
	//	Renderer2D::DrawQuad({ -1.0f, 0.0f }, { 0.8f, 0.8f }, { 0.8f, 0.2f, 0.3f, 1.0f });
	//	Renderer2D::DrawQuad({ 0.5f, -0.5f }, { 0.5f, 0.75f }, { 0.2f, 0.3f, 0.8f, 1.0f });
	//	Renderer2D::DrawQuad({ 0.0f, 0.0f, -0.1f }, { 20.0f, 20.0f }, m_CheckerboardTexture, 10.0f);
	//	Renderer2D::DrawRotatedQuad({ -2.0f, 0.0f, 0.0f }, { 1.0f, 1.0f }, glm::radians(rotation), m_CheckerboardTexture, 20.0f);

	//	Renderer2D::DrawQuad({ 0.0f, 0.0f, 0.1f }, { 1.0f, 1.0f }, m_TextureStairs);
	//	Renderer2D::DrawQuad({ 1.0f, 0.0f, 0.1f }, { 1.0f, 1.0f }, m_TextureBarrel);
	//	Renderer2D::DrawRotatedQuad({ -1.0f, 0.0f, 0.1f }, { 1.0f, 2.0f }, glm::radians(rotation), m_TextureTree, { 0.8f, 0.2f, 0.3f, 0.25f });
	//	Renderer2D::EndScene();
	//}
}

void SandboxLayer::OnEvent(Event& e)
{
	EventDispatcher dispatcher(e);
	dispatcher.Dispatch<WindowResizeEvent>(RADIANT_BIND_EVENT_FN(SandboxLayer::OnWindowResized));
}

bool SandboxLayer::OnWindowResized(WindowResizeEvent& e)
{
	RADIANT_PROFILE_FUNCTION();

	m_Level->OnViewportResize(e.GetWidth(), e.GetHeight());
	return false;
}
