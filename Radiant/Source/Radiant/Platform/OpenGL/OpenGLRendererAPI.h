#pragma once

#include "Radiant/Renderer/RendererAPI.h"

namespace Radiant {

	/**
	 * Immediate-mode GL backend: every call translates directly to GL against
	 * global context state — no batching or deferral at this layer. Init()
	 * enables alpha blending and depth testing, plus GL debug output in Debug
	 * builds.
	 */
	class OpenGLRendererAPI : public RendererAPI
	{
	public:
		virtual void Init() override;
		virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;

		virtual void SetClearColor(const glm::vec4& color) override;
		virtual void Clear() override;

		virtual void DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount = 0) override;
		virtual void DrawLines(const Ref<VertexArray>& vertexArray, uint32_t vertexCount) override;

		virtual void SetLineWidth(float width) override;
	};

}