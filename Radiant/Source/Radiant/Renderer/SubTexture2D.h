#pragma once

#include "Texture.h"

namespace Radiant {

	/**
	 * A rectangular region of a texture atlas, stored as normalized UV
	 * coordinates. Ref-counted; holds a Ref to the atlas texture, keeping it
	 * alive for the subtexture's lifetime — no pixel data is copied.
	 */
	class SubTexture2D : public RefCounted
	{
	public:
		/** min/max are normalized UV corners of the region within `texture`. */
		SubTexture2D(const Ref<Texture2D>& texture, const glm::vec2& min, const glm::vec2& max);

		const Ref<Texture2D> GetTexture() const { return m_Texture; }
		/**
		 * The four UV corners (bottom-left, bottom-right, top-right, top-left).
		 * The pointer aliases internal storage — valid only while this object
		 * lives.
		 */
		const glm::vec2* GetTexCoords() const { return m_TexCoords; }

		/**
		 * Builds a subtexture from a sprite-sheet grid: `coords` is the cell
		 * position, `cellSize` the cell dimensions in pixels, and `spriteSize`
		 * how many cells the sprite spans.
		 */
		static Ref<SubTexture2D> CreateFromCoords(const Ref<Texture2D>& texture, const glm::vec2& coords, const glm::vec2& cellSize, const glm::vec2& spriteSize = { 1, 1 });
	private:
		Ref<Texture2D> m_Texture;

		glm::vec2 m_TexCoords[4];
	};
}