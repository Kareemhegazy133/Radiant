#pragma once

#include "Level.h"

#include "Serialization/FileStream.h"
#include "Asset/AssetSerializer.h"

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
		bool SerializeToAssetPack(FileStreamWriter& stream, AssetSerializationInfo& outInfo);

		bool Deserialize(const std::filesystem::path& filepath);
		bool DeserializeFromYAML(const std::string& yamlString);
		bool DeserializeFromAssetPack(FileStreamReader& stream, const AssetPackFile::LevelInfo& levelInfo);

	public:
		static void SerializeEntity(YAML::Emitter& out, Entity entity);
		static void DeserializeEntities(YAML::Node& entitiesNode, Ref<Level> level);

	private:
		Ref<Level> m_Level;
	};

}