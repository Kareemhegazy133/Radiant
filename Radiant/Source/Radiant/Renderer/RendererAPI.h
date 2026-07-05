#pragma once

#include <glm/glm.hpp>

#include "Radiant/Renderer/VertexArray.h"

namespace Radiant {

	/**
	 * Abstract graphics backend — the low-level clear/viewport/draw vocabulary
	 * each API implements. Exactly one instance exists, owned (Scope) by
	 * RenderCommand; call through RenderCommand rather than holding a backend
	 * pointer. The active API is a hardcoded static (OpenGL) that Create()
	 * switches on. Main-thread only; calls execute immediately.
	 */
	class RendererAPI
	{
	public:
		enum class API
		{
			None = 0, OpenGL = 1
		};

	public:
		virtual ~RendererAPI() = default;

		/** Sets global pipeline state (blending, depth test). Requires a live context. */
		virtual void Init() = 0;
		/** Viewport rectangle in pixels, origin bottom-left. */
		virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;
		virtual void SetClearColor(const glm::vec4& color) = 0;
		/** Clears the bound target's color and depth buffers. */
		virtual void Clear() = 0;

		/** Draws indexed triangles; indexCount == 0 means the array's whole index buffer. */
		virtual void DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount = 0) = 0;
		/** Draws vertexCount/2 unindexed line segments from the array's vertex buffer. */
		virtual void DrawLines(const Ref<VertexArray>& vertexArray, uint32_t vertexCount) = 0;

		/** Width in pixels applied to subsequent line draws. */
		virtual void SetLineWidth(float width) = 0;

		static API GetAPI() { return s_API; }
		static Scope<RendererAPI> Create();
	private:
		static API s_API;
	};

}