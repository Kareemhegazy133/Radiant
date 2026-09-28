#pragma once

#include <Radiant/Radiant.h>

// Reaper opts into the engine namespace (game-local choice; the engine no longer injects it)
using namespace Radiant;



/**
 * Native script giving the camera entity WASD movement and Q/E roll, applied
 * to its TransformComponent every update. Speeds are per-second (rotation in
 * radians), scaled by ts for framerate independence. Position/rotation are
 * cached from the transform once in OnCreate — external transform edits made
 * afterwards get overwritten each frame. Bindings are code-only: GameLayer
 * re-binds this to the "Camera" entity after level load.
 */
class CameraController : public EntityBehaviour
{
public:
	void OnCreate() override
	{
		// Named verbs rather than reaching through TransformComponent: the
		// storage detail is not this script's business, and asking "where am I"
		// should not require knowing which component answers (RAD-99)
		m_CameraPosition = GetOwner().GetLocation();
		m_CameraRotation = GetOwner().GetRotation();

		m_CameraTranslationSpeed = 5.0f; // Movement speed
		m_CameraRotationSpeed = glm::radians(45.0f); // Rotation speed (radians per second)
	}

	void OnUpdate(Timestep ts) override
	{
		// Handle rotation (left and right)
		if (Input::IsKeyPressed(Key::Q)) // Rotate left
			m_CameraRotation += m_CameraRotationSpeed * ts;
		if (Input::IsKeyPressed(Key::E)) // Rotate right
			m_CameraRotation -= m_CameraRotationSpeed * ts;

		// Handle translation (W, A, S, D) based on camera's rotation
		if (Input::IsKeyPressed(Key::A)) // Move left
		{
			m_CameraPosition.x -= cos(m_CameraRotation) * m_CameraTranslationSpeed * ts;
			m_CameraPosition.y -= sin(m_CameraRotation) * m_CameraTranslationSpeed * ts;
		}
		else if (Input::IsKeyPressed(Key::D)) // Move right
		{
			m_CameraPosition.x += cos(m_CameraRotation) * m_CameraTranslationSpeed * ts;
			m_CameraPosition.y += sin(m_CameraRotation) * m_CameraTranslationSpeed * ts;
		}

		if (Input::IsKeyPressed(Key::W)) // Move forward
		{
			m_CameraPosition.x += -sin(m_CameraRotation) * m_CameraTranslationSpeed * ts;
			m_CameraPosition.y += cos(m_CameraRotation) * m_CameraTranslationSpeed * ts;
		}
		else if (Input::IsKeyPressed(Key::S)) // Move backward
		{
			m_CameraPosition.x -= -sin(m_CameraRotation) * m_CameraTranslationSpeed * ts;
			m_CameraPosition.y -= cos(m_CameraRotation) * m_CameraTranslationSpeed * ts;
		}

		// SetTransform, not SetLocation + SetRotation: this runs every fixed
		// step, and each of the single-axis verbs has to read the half it is
		// preserving, so the pair would cost four transform lookups where this
		// costs one (Entity.h states the rule).
		//
		// SetLocation-family verbs preserve the render snapshot, which is what
		// keeps this camera smooth: OnRender draws lerp(snapshot, current,
		// alpha), so the motion is interpolated across the frames between two
		// simulation steps. Routing this through Teleport instead would stamp
		// the snapshot every step and pin the camera to fixed-step granularity
		// — visible judder at any display rate above the sim rate.
		GetOwner().SetTransform(m_CameraPosition, m_CameraRotation);
	}

private:
	glm::vec3 m_CameraPosition;
	float m_CameraRotation;
	float m_CameraTranslationSpeed;
	float m_CameraRotationSpeed;
};
