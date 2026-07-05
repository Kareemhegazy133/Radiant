#pragma once

#include "Level.h"

#include "Radiant/Serialization/FileStream.h"
#include "Radiant/Asset/AssetSerializerAPI.h"

namespace YAML {
	class Emitter;
	class Node;
}

namespace Radiant {

	/**
	 * (De)serializes a Level to the YAML .rdlvl format: a level name plus one map
	 * per entity, keyed by UUID and emitted in UUID order for stable diffs. Holds
	 * a Ref to the target level; does not own the streams passed in.
	 *
	 * The round-trip is lossy where noted: MetadataComponent::IsActive and native
	 * script bindings are not serialized (bindings are code-only — re-bind after
	 * load). Asset references travel as handles and are resolved through the
	 * AssetManager on load. Deserialization adds physics components last so their
	 * entt construct signals fire after the transform exists.
	 */
	class LevelSerializer
	{
	public:
		LevelSerializer(const Ref<Level>& level);

		/** Writes the level as YAML to filepath. File I/O errors are currently silent (no return value). */
		void Serialize(const std::filesystem::path& filepath);
		void SerializeToYAML(YAML::Emitter& out);
		/** Emits the level's YAML as a string blob into the pack stream, recording offset/size in outInfo. */
		bool SerializeToAssetPack(FileStreamWriter& stream, AssetSerializationInfo& outInfo);

		/**
		 * Loads a .rdlvl file into the target level. Returns false on YAML parse
		 * exceptions; asserts on an unreadable file. Note: DeserializeFromYAML's
		 * own result is currently dropped, so a well-formed file without a "Level"
		 * key still returns true (known honest-error-propagation gap).
		 */
		bool Deserialize(const std::filesystem::path& filepath);
		/** Populates the target level from a YAML string; false when the "Level" key is missing. Entities are created via CreateEntityWithUUID. */
		bool DeserializeFromYAML(const std::string& yamlString);
		bool DeserializeFromAssetPack(FileStreamReader& stream, const AssetPackFile::LevelInfo& levelInfo);

	public:
		static void SerializeEntity(YAML::Emitter& out, Entity entity);
		static void DeserializeEntities(YAML::Node& entitiesNode, Ref<Level> level);

	private:
		Ref<Level> m_Level;
	};

}