#pragma once

#include "Level.h"

namespace YAML {
	class Emitter;
	class Node;
}

namespace Radiant {

	class LevelSerializer
	{
	public:
		LevelSerializer(const Ref<Level>& level);

		void Serialize(const std::filesystem::path& filepath);
		void SerializeToYAML(YAML::Emitter& out);

		bool Deserialize(const std::filesystem::path& filepath);
		bool DeserializeFromYAML(const std::string& yamlString);

	public:
		static void SerializeEntity(YAML::Emitter& out, Entity entity);
		static void DeserializeEntities(YAML::Node& entitiesNode, Ref<Level> level);

	private:
		Ref<Level> m_Level;
	};

}