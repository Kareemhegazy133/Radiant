#pragma once

#include "Radiant/Renderer/Buffer.h"

namespace Radiant {

	/**
	 * Binds vertex buffers (with their layouts) and an index buffer into one
	 * drawable set. This is a GL-only concept promoted to an engine type — it
	 * has no Vulkan equivalent and is deleted with the GL backend at the end of
	 * Phase 3 (RAD-42). Holds Refs to its buffers, keeping them alive.
	 * Ref-counted; must be released before the graphics context; main-thread
	 * only.
	 */
	class VertexArray : public RefCounted
	{
	public:
		virtual ~VertexArray() = default;

		virtual void Bind() const = 0;
		virtual void Unbind() const = 0;

		/** The buffer's layout must already be set (asserted). */
		virtual void AddVertexBuffer(const Ref<VertexBuffer>& vertexBuffer) = 0;
		virtual void SetIndexBuffer(const Ref<IndexBuffer>& indexBuffer) = 0;

		virtual const std::vector<Ref<VertexBuffer>>& GetVertexBuffers() const = 0;
		virtual const Ref<IndexBuffer>& GetIndexBuffer() const = 0;

		static Ref<VertexArray> Create();
	};

}