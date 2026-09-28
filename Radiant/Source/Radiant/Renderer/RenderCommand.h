#pragma once

#include "RendererAPI.h"

namespace Radiant {

	/**
	 * Static facade over the single global RendererAPI backend. There is no
	 * command buffer and no render thread: every call inline-forwards through
	 * one virtual dispatch and executes on the calling thread immediately —
	 * which must be the main thread, with a live graphics context.
	 */
	class RenderCommand
	{
	public:
		static void Init()
		{
			s_RendererAPI->Init();
		}

		static void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
		{
			s_RendererAPI->SetViewport(x, y, width, height);
		}

		static void SetClearColor(const glm::vec4& color)
		{
			s_RendererAPI->SetClearColor(color);
		}

		static void Clear()
		{
			s_RendererAPI->Clear();
		}

		/** indexCount == 0 draws the vertex array's entire index buffer. */
		static void DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount = 0)
		{
			s_RendererAPI->DrawIndexed(vertexArray, indexCount);
		}

		static void DrawLines(const Ref<VertexArray>& vertexArray, uint32_t vertexCount)
		{
			s_RendererAPI->DrawLines(vertexArray, vertexCount);
		}

		static void SetLineWidth(float width)
		{
			s_RendererAPI->SetLineWidth(width);
		}

	private:
		static Scope<RendererAPI> s_RendererAPI;
	};

}