#pragma once

#include "Camera.h"

#include "Texture.h"
#include "SubTexture2D.h"

#include "Radiant/ECS/Components.h"

namespace Radiant {

	/**
	 * Batched 2D drawing facade — the API games render through.
	 *
	 * Draw calls never touch the GPU directly: they append vertices to a CPU-side
	 * batch that is uploaded and drawn when the batch flushes. Every frame's draws
	 * must be bracketed by BeginScene()/EndScene(); scenes cannot be nested —
	 * there is a single global batch.
	 *
	 * A batch flushes when EndScene() runs, when it reaches capacity (20,000 quads
	 * or 80,000 line vertices), or when a quad references a 32nd unique texture in
	 * one batch — slot 0 is reserved for the built-in 1x1 white texture used by
	 * untextured quads, leaving 31 user texture slots. Each flush issues one draw
	 * call per primitive family (quads, lines).
	 *
	 * Init() creates the GPU resources and requires a live graphics context — it is
	 * called by Renderer::Init(). Textures passed to draw calls are retained (Ref)
	 * at least until the batch referencing them is flushed. Like the rest of the
	 * renderer, this API is main-thread only.
	 */
	class Renderer2D : public RefCounted
	{
	public:
		static void Init();
		static void Shutdown();

		/**
		 * Begins a new scene: uploads the camera's view-projection to the shared
		 * camera UBO (binding 0) and resets the batch.
		 *
		 * @param transform The camera's world transform (not a view matrix) — it
		 *                  is inverted here to produce the view.
		 */
		static void BeginScene(const Camera& camera, const glm::mat4& transform);
		/** Submits everything drawn since BeginScene() by flushing the batch. */
		static void EndScene();
		/**
		 * Uploads the used range of each primitive stream and issues the draw
		 * calls. Runs automatically from EndScene() and when a batch fills
		 * mid-scene; it does not reset the batch cursors, so external callers
		 * should use EndScene() instead.
		 */
		static void Flush();

		// Primitives

		/**
		 * Appends an axis-aligned quad centered on `position`; `size` is in world
		 * units. Overloads taking a texture (or atlas subtexture) sample it across
		 * the quad multiplied by `tintColor`; `tilingFactor` scales the texture
		 * coordinates, so values > 1 repeat the texture. The vec2 position
		 * overloads draw at z = 0.
		 */
		static void DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color);
		static void DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color);
		static void DrawQuad(const glm::vec2& position, const glm::vec2& size, const Ref<Texture2D>& texture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f));
		static void DrawQuad(const glm::vec3& position, const glm::vec2& size, const Ref<Texture2D>& texture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f));
		static void DrawQuad(const glm::vec2& position, const glm::vec2& size, const Ref<SubTexture2D>& subtexture, const glm::vec4& tintColor = glm::vec4(1.0f));
		static void DrawQuad(const glm::vec3& position, const glm::vec2& size, const Ref<SubTexture2D>& subtexture, const glm::vec4& tintColor = glm::vec4(1.0f));

		/**
		 * Appends a unit quad (centered on the origin) transformed by an arbitrary
		 * model matrix — the lowest-level quad path every other overload funnels
		 * into.
		 */
		static void DrawQuad(const glm::mat4& transform, const glm::vec4& color);
		static void DrawQuad(const glm::mat4& transform, const Ref<Texture2D>& texture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f));
		static void DrawQuad(const glm::mat4& transform, const Ref<SubTexture2D>& subtexture, const glm::vec4& tintColor = glm::vec4(1.0f));

		/** Same contract as DrawQuad; `rotation` is in radians, about the z-axis. */
		static void DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const glm::vec4& color);
		static void DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const glm::vec4& color);
		static void DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const Ref<Texture2D>& texture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f));
		static void DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const Ref<Texture2D>& texture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f));
		static void DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const Ref<SubTexture2D>& subtexture, const glm::vec4& tintColor = glm::vec4(1.0f));
		static void DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const Ref<SubTexture2D>& subtexture, const glm::vec4& tintColor = glm::vec4(1.0f));
	
		/** Appends a line segment between two world-space points. */
		static void DrawLine(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color);

		/** Appends a rectangle outline as four lines — not a filled quad. */
		static void DrawRect(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color);
		static void DrawRect(const glm::mat4& transform, const glm::vec4& color);

		/** Entity sprite path — forwards to the matching DrawQuad overload. */
		static void DrawSprite(const glm::mat4& transform, const Ref<Texture2D>& texture, float tilingFactor = 1.0f, const glm::vec4& color = glm::vec4(1.0f));
		static void DrawSprite(const glm::mat4& transform, const glm::vec4& color = glm::vec4(1.0f));

		/**
		 * Line width in pixels. The width is bound once per flush, not recorded
		 * per line — the value current at flush time applies to every line in
		 * that batch.
		 */
		static float GetLineWidth();
		static void SetLineWidth(float width);
	private:
		static void StartBatch();
		static void NextBatch();
	};

}