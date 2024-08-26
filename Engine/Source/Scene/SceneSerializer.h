#pragma once

#include "Scene/Scene.h"

namespace Engine {

	class SceneSerializer
	{
	public:
		SceneSerializer(Scene* scene);

		void Serialize(const std::string& filepath);
		bool Deserialize(const std::string& filepath);

	private:
		Scene* m_Scene;
	};
}