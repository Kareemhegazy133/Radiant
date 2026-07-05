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
class CameraController : public ScriptableEntity
{
public:
	void OnCreate()
	{
		// Get camera position and rotation from TransformComponent
		m_CameraPosition = GetComponent<TransformComponent>().Translation;
		m_CameraRotation = GetComponent<TransformComponent>().Rotation.z;
		m_CameraTranslationSpeed = 5.0f; // Movement speed
		m_CameraRotationSpeed = glm::radians(45.0f); // Rotation speed (radians per second)
	}

	void OnUpdate(Timestep ts)
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

		// Apply the calculated position to the entity's transform
		auto& transform = GetComponent<TransformComponent>();
		transform.Translation = m_CameraPosition; // Update the translation

		// You could also apply rotation to the transform if needed:
		transform.Rotation.z = m_CameraRotation;
	}

private:
	glm::vec3 m_CameraPosition;
	float m_CameraRotation;
	float m_CameraTranslationSpeed;
	float m_CameraRotationSpeed;
};
