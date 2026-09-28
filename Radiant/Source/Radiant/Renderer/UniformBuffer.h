#pragma once

#include "Radiant/Core/Base.h"

namespace Radiant {

	/**
	 * GPU uniform buffer bound to a fixed binding point for its whole lifetime;
	 * `binding` is chosen at Create and must match the shader's
	 * layout(binding = N) block. Ref-counted; must be released before the
	 * graphics context; main-thread only.
	 */
	class UniformBuffer : public RefCounted
	{
	public:
		virtual ~UniformBuffer() {}
		/**
		 * Uploads `size` bytes to `offset` bytes into the buffer, synchronously —
		 * the caller keeps ownership of `data` and may free it on return.
		 */
		virtual void SetData(const void* data, uint32_t size, uint32_t offset = 0) = 0;

		static Ref<UniformBuffer> Create(uint32_t size, uint32_t binding);
	};

}