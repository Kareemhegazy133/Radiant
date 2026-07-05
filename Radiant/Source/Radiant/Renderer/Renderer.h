#pragma once

#include "RenderCommand.h"

#include "SceneCamera.h"
#include "Shader.h"

namespace Radiant {

	/**
	 * Top-level renderer lifecycle facade. Init() boots the backend
	 * (RenderCommand::Init) and the 2D batcher (Renderer2D::Init) and therefore
	 * requires a live graphics context — the window must exist first.
	 * Main-thread only.
	 *
	 * BeginScene/EndScene/Submit are the legacy per-object draw path: Submit
	 * sets uniforms by string name and is incompatible with the 450-core
	 * Renderer2D shaders — it dies with the GL backend in Phase 3. Game code
	 * draws through Renderer2D.
	 */
	class Renderer
	{
	public:
		static void Init();
		static void Shutdown();

		/** Resizes the viewport to the new framebuffer size in pixels. */
		static void OnWindowResize(uint32_t width, uint32_t height);

		static void BeginScene(SceneCamera& camera);
		static void EndScene();

		static void Submit(const Ref<Shader>& shader, const Ref<VertexArray>& vertexArray, const glm::mat4& transform = glm::mat4(1.0f));

		static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }
	private:
		struct SceneData
		{
			glm::mat4 ViewProjectionMatrix;
		};

		static Scope<SceneData> s_SceneData;
	};
}