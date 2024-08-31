#pragma once

#include "ECS/Level.h"

namespace Engine {

	class LevelSerializer
	{
	public:
		LevelSerializer(Level* scene);

		void Serialize(const std::string& filepath);
		bool Deserialize(const std::string& filepath);

	private:
		Level* m_Level;
	};
}