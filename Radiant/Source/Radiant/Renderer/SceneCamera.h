#pragma once

#include "Camera.h"

namespace Radiant {

	/**
	 * The camera entities carry (CameraComponent): a Camera whose projection is
	 * rebuilt from serializable parameters whenever one changes.
	 *
	 * Units: perspective FOV is the vertical field of view, stored in degrees;
	 * orthographic size is the full vertical extent of the view volume in world
	 * units (width follows from the aspect ratio). Plain value type — copied
	 * with its component.
	 */
	class SceneCamera : public Camera
	{
	public:
		enum class ProjectionType { Perspective = 0, Orthographic = 1 };
	public:
		SceneCamera();
		virtual ~SceneCamera() = default;

		/** Switches to perspective projection. verticalFOV is in degrees. */
		void SetPerspective(float verticalFOV, float nearClip, float farClip);
		/** Switches to orthographic projection. size is the vertical extent in world units. */
		void SetOrthographic(float size, float nearClip, float farClip);

		/** Feeds the aspect ratio from viewport dimensions in pixels; both must be non-zero (asserted). */
		void SetViewportSize(uint32_t width, uint32_t height);

		void SetAspectRatio(float aspectRatio) { m_AspectRatio = aspectRatio; RecalculateProjection(); }
		const float GetAspectRatio() const { return m_AspectRatio; }

		// NOTE: unlike every other setter, the Deg/Rad FOV setters do not rebuild
		// the projection — the new FOV takes effect on the next recalculation.
		// The Rad variant takes radians (its parameter name is stale) and stores
		// degrees.
		void SetDegPerspectiveVerticalFOV(const float degVerticalFov) { m_DegPerspectiveFOV = degVerticalFov; }
		void SetRadPerspectiveVerticalFOV(const float degVerticalFov) { m_DegPerspectiveFOV = glm::degrees(degVerticalFov); }
		float GetDegPerspectiveVerticalFOV() const { return m_DegPerspectiveFOV; }
		float GetRadPerspectiveVerticalFOV() const { return glm::radians(m_DegPerspectiveFOV); }
		void SetPerspectiveVerticalFOV(float verticalFov) { m_DegPerspectiveFOV = verticalFov; RecalculateProjection(); }
		float GetPerspectiveNearClip() const { return m_PerspectiveNear; }
		void SetPerspectiveNearClip(float nearClip) { m_PerspectiveNear = nearClip; RecalculateProjection(); }
		float GetPerspectiveFarClip() const { return m_PerspectiveFar; }
		void SetPerspectiveFarClip(float farClip) { m_PerspectiveFar = farClip; RecalculateProjection(); }

		float GetOrthographicSize() const { return m_OrthographicSize; }
		void SetOrthographicSize(float size) { m_OrthographicSize = size; RecalculateProjection(); }
		float GetOrthographicNearClip() const { return m_OrthographicNear; }
		void SetOrthographicNearClip(float nearClip) { m_OrthographicNear = nearClip; RecalculateProjection(); }
		float GetOrthographicFarClip() const { return m_OrthographicFar; }
		void SetOrthographicFarClip(float farClip) { m_OrthographicFar = farClip; RecalculateProjection(); }

		ProjectionType GetProjectionType() const { return m_ProjectionType; }
		void SetProjectionType(ProjectionType type) { m_ProjectionType = type; RecalculateProjection(); }
	private:
		void RecalculateProjection();
	private:
		ProjectionType m_ProjectionType = ProjectionType::Orthographic;

		float m_DegPerspectiveFOV = 45.0f;
		float m_PerspectiveNear = 0.01f, m_PerspectiveFar = 1000.0f;

		float m_OrthographicSize = 10.0f;
		float m_OrthographicNear = -1.0f, m_OrthographicFar = 1.0f;

		float m_AspectRatio = 0.0f;
	};

}