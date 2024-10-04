#pragma once

#include <Radiant.h>

using namespace Radiant;

class CameraController : public ScriptableEntity
{
public:
	void OnCreate()
	{
		// Initialize camera position and rotation if necessary
		m_CameraPosition = glm::vec3(0.0f, 0.0f, 0.0f);
		m_CameraRotation = 0.0f; // Initial camera rotation
		m_CameraTranslationSpeed = 5.0f; // Movement speed
		m_CameraRotationSpeed = 45.0f; // Rotation speed (degrees per second)
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
			m_CameraPosition.x -= cos(glm::radians(m_CameraRotation)) * m_CameraTranslationSpeed * ts;
			m_CameraPosition.y -= sin(glm::radians(m_CameraRotation)) * m_CameraTranslationSpeed * ts;
		}
		else if (Input::IsKeyPressed(Key::D)) // Move right
		{
			m_CameraPosition.x += cos(glm::radians(m_CameraRotation)) * m_CameraTranslationSpeed * ts;
			m_CameraPosition.y += sin(glm::radians(m_CameraRotation)) * m_CameraTranslationSpeed * ts;
		}

		if (Input::IsKeyPressed(Key::W)) // Move forward
		{
			m_CameraPosition.x += -sin(glm::radians(m_CameraRotation)) * m_CameraTranslationSpeed * ts;
			m_CameraPosition.y += cos(glm::radians(m_CameraRotation)) * m_CameraTranslationSpeed * ts;
		}
		else if (Input::IsKeyPressed(Key::S)) // Move backward
		{
			m_CameraPosition.x -= -sin(glm::radians(m_CameraRotation)) * m_CameraTranslationSpeed * ts;
			m_CameraPosition.y -= cos(glm::radians(m_CameraRotation)) * m_CameraTranslationSpeed * ts;
		}

		// Apply the calculated position to the entity's transform
		auto& transform = GetComponent<TransformComponent>();
		transform.Translation = m_CameraPosition; // Update the translation

		// You could also apply rotation to the transform if needed:
		transform.Rotation.z = glm::radians(m_CameraRotation);
	}

private:
	glm::vec3 m_CameraPosition;
	float m_CameraRotation;
	float m_CameraTranslationSpeed;
	float m_CameraRotationSpeed;
};
