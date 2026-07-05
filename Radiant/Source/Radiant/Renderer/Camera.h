#pragma once

#include <glm/glm.hpp>

namespace Radiant {

	/**
	 * Minimal camera base: a projection matrix and nothing else. No view matrix
	 * lives here — the renderer derives the view by inverting the camera
	 * entity's world transform at Renderer2D::BeginScene. Plain value type;
	 * derived cameras (SceneCamera) rebuild m_Projection when their parameters
	 * change.
	 */
	class Camera
	{
	public:
		Camera() = default;
		Camera(const glm::mat4& projection)
			: m_Projection(projection) {}

		virtual ~Camera() = default;

		const glm::mat4& GetProjection() const { return m_Projection; }

	protected:
		glm::mat4 m_Projection = glm::mat4(1.0f);
	};

}