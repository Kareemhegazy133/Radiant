#include "rdpch.h"
#include "LevelSerializer.h"

#include <yaml-cpp/yaml.h>
#include <fstream>

#include "Core/UUID.h"
#include "Entity.h"
#include "Components.h"

#include "Utilities/YAMLSerializationHelpers.h"

namespace Radiant {

	static std::string RigidBody2DBodyTypeToString(RigidBody2DComponent::BodyType bodyType)
	{
		switch (bodyType)
		{
		case RigidBody2DComponent::BodyType::Static:    return "Static";
		case RigidBody2DComponent::BodyType::Dynamic:   return "Dynamic";
		case RigidBody2DComponent::BodyType::Kinematic: return "Kinematic";
		}

		RADIANT_ASSERT(false, "Unknown body type");
		return {};
	}

	static RigidBody2DComponent::BodyType RigidBody2DBodyTypeFromString(const std::string& bodyTypeString)
	{
		if (bodyTypeString == "Static")    return RigidBody2DComponent::BodyType::Static;
		if (bodyTypeString == "Dynamic")   return RigidBody2DComponent::BodyType::Dynamic;
		if (bodyTypeString == "Kinematic") return RigidBody2DComponent::BodyType::Kinematic;

		RADIANT_ASSERT(false, "Unknown body type");
		return RigidBody2DComponent::BodyType::Static;
	}

	LevelSerializer::LevelSerializer(const Ref<Level>& level)
		: m_Level(level)
	{
	}

	void LevelSerializer::Serialize(const std::filesystem::path& filepath)
	{
		YAML::Emitter out;
		SerializeToYAML(out);

		std::ofstream fout(filepath);
		fout << out.c_str();
	}

	void LevelSerializer::SerializeToYAML(YAML::Emitter& out)
	{
		out << YAML::BeginMap;
		out << YAML::Key << "Level" << YAML::Value << m_Level->GetName();

		out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;

		// Sort entities by UUID (for better serializing)
		std::map<UUID, entt::entity> sortedEntityMap;
		auto view = m_Level->m_Registry.view<MetadataComponent>();
		for (auto entity : view)
			sortedEntityMap[view.get<MetadataComponent>(entity).ID] = entity;

		// Serialize sorted entities
		for (auto [id, entity] : sortedEntityMap)
			SerializeEntity(out, { entity, m_Level.get() });

		out << YAML::EndSeq;
		out << YAML::EndMap;
	}

	bool LevelSerializer::SerializeToAssetPack(FileStreamWriter& stream, AssetSerializationInfo& outInfo)
	{
		YAML::Emitter out;
		SerializeToYAML(out);

		outInfo.Offset = stream.GetStreamPosition();
		std::string yamlString = out.c_str();
		stream.WriteString(yamlString);
		outInfo.Size = stream.GetStreamPosition() - outInfo.Offset;
		return true;
	}

	void LevelSerializer::SerializeEntity(YAML::Emitter& out, Entity entity)
	{
		RADIANT_ASSERT(entity.HasComponent<MetadataComponent>());

		out << YAML::BeginMap; // Entity
		out << YAML::Key << "Entity" << YAML::Value << entity.GetUUID();

		if (entity.HasComponent<MetadataComponent>())
		{
			out << YAML::Key << "MetadataComponent";
			out << YAML::BeginMap; // MetadataComponent

			auto& tag = entity.GetComponent<MetadataComponent>().Tag;
			out << YAML::Key << "Tag" << YAML::Value << tag;

			out << YAML::EndMap; // MetadataComponent
		}

		if (entity.HasComponent<TransformComponent>())
		{
			out << YAML::Key << "TransformComponent";
			out << YAML::BeginMap; // TransformComponent

			auto& tc = entity.GetComponent<TransformComponent>();
			out << YAML::Key << "Translation" << YAML::Value << tc.Translation;
			out << YAML::Key << "Rotation" << YAML::Value << tc.Rotation;
			out << YAML::Key << "Scale" << YAML::Value << tc.Scale;

			out << YAML::EndMap; // TransformComponent
		}

		if (entity.HasComponent<CameraComponent>())
		{
			out << YAML::Key << "CameraComponent";
			out << YAML::BeginMap; // CameraComponent

			auto& cameraComponent = entity.GetComponent<CameraComponent>();
			auto& camera = cameraComponent.Camera;

			out << YAML::Key << "Camera" << YAML::Value;
			out << YAML::BeginMap; // Camera
			out << YAML::Key << "ProjectionType" << YAML::Value << (int)camera.GetProjectionType();
			out << YAML::Key << "AspectRatio" << YAML::Value << camera.GetAspectRatio();
			out << YAML::Key << "PerspectiveFOV" << YAML::Value << camera.GetDegPerspectiveVerticalFOV();
			out << YAML::Key << "PerspectiveNear" << YAML::Value << camera.GetPerspectiveNearClip();
			out << YAML::Key << "PerspectiveFar" << YAML::Value << camera.GetPerspectiveFarClip();
			out << YAML::Key << "OrthographicSize" << YAML::Value << camera.GetOrthographicSize();
			out << YAML::Key << "OrthographicNear" << YAML::Value << camera.GetOrthographicNearClip();
			out << YAML::Key << "OrthographicFar" << YAML::Value << camera.GetOrthographicFarClip();
			out << YAML::EndMap; // Camera

			out << YAML::Key << "Primary" << YAML::Value << cameraComponent.Primary;
			out << YAML::Key << "FixedAspectRatio" << YAML::Value << cameraComponent.FixedAspectRatio;

			out << YAML::EndMap; // CameraComponent
		}

		if (entity.HasComponent<SpriteComponent>())
		{
			out << YAML::Key << "SpriteComponent";
			out << YAML::BeginMap; // SpriteComponent

			auto& spriteComponent = entity.GetComponent<SpriteComponent>();
			out << YAML::Key << "Color" << YAML::Value << spriteComponent.Color;
			out << YAML::Key << "TextureHandle" << YAML::Value << spriteComponent.Texture;
			out << YAML::Key << "TilingFactor" << YAML::Value << spriteComponent.TilingFactor;

			out << YAML::EndMap; // SpriteComponent
		}

		if (entity.HasComponent<NativeScriptComponent>())
		{
			out << YAML::Key << "NativeScriptComponent";
			out << YAML::BeginMap; // NativeScriptComponent

			// TODO: Reflection system?
			/*auto& nativeScriptComponent = entity.GetComponent<NativeScriptComponent>();
			out << YAML::Key << "Script Class Name" << YAML::Value << nativeScriptComponent.ScriptClass.name();*/

			out << YAML::EndMap; // NativeScriptComponent
		}

		if (entity.HasComponent<RigidBody2DComponent>())
		{
			out << YAML::Key << "RigidBody2DComponent";
			out << YAML::BeginMap; // RigidBody2DComponent

			auto& rb2dComponent = entity.GetComponent<RigidBody2DComponent>();
			out << YAML::Key << "BodyType" << YAML::Value << RigidBody2DBodyTypeToString(rb2dComponent.Type);
			out << YAML::Key << "FixedRotation" << YAML::Value << rb2dComponent.FixedRotation;

			out << YAML::EndMap; // RigidBody2DComponent
		}

		if (entity.HasComponent<BoxCollider2DComponent>())
		{
			out << YAML::Key << "BoxCollider2DComponent";
			out << YAML::BeginMap; // BoxCollider2DComponent

			auto& bc2dComponent = entity.GetComponent<BoxCollider2DComponent>();
			out << YAML::Key << "Offset" << YAML::Value << bc2dComponent.Offset;
			out << YAML::Key << "Size" << YAML::Value << bc2dComponent.Size;
			out << YAML::Key << "Density" << YAML::Value << bc2dComponent.Density;
			out << YAML::Key << "Friction" << YAML::Value << bc2dComponent.Friction;
			out << YAML::Key << "Restitution" << YAML::Value << bc2dComponent.Restitution;
			out << YAML::Key << "RestitutionThreshold" << YAML::Value << bc2dComponent.RestitutionThreshold;

			out << YAML::EndMap; // BoxCollider2DComponent
		}

		out << YAML::EndMap; // Entity
	}

	bool LevelSerializer::Deserialize(const std::filesystem::path& filepath)
	{
		std::ifstream stream(filepath);
		RADIANT_ASSERT(stream);
		std::stringstream strStream;
		strStream << stream.rdbuf();

		try
		{
			DeserializeFromYAML(strStream.str());
		}
		catch (const YAML::Exception& e)
		{
			RADIANT_ERROR("Failed to deserialize level '{0}': {1}", filepath.string(), e.what());
			return false;
		}

		return true;
	}

	bool LevelSerializer::DeserializeFromYAML(const std::string& yamlString)
	{
		YAML::Node data = YAML::Load(yamlString);

		if (!data["Level"])
			return false;

		std::string levelName = data["Level"].as<std::string>();
		RADIANT_INFO("Deserializing level '{0}'", levelName);
		m_Level->SetName(levelName);

		auto entities = data["Entities"];
		if (entities)
			DeserializeEntities(entities, m_Level);

		// Sort MetadataComponent by entity handle (which is essentially the order in which they were created)
		// This ensures a consistent ordering when iterating MetadataComponent
		m_Level->m_Registry.sort<MetadataComponent>([this](const auto lhs, const auto rhs)
			{
				auto lhsEntity = m_Level->m_EntityMap.find(lhs.ID);
				auto rhsEntity = m_Level->m_EntityMap.find(rhs.ID);
				return static_cast<uint32_t>(lhsEntity->second) < static_cast<uint32_t>(rhsEntity->second);
			});

		return true;
	}

	bool LevelSerializer::DeserializeFromAssetPack(FileStreamReader& stream, const AssetPackFile::LevelInfo& levelInfo)
	{
		stream.SetStreamPosition(levelInfo.PackedOffset);
		std::string levelYAML;
		stream.ReadString(levelYAML);

		return DeserializeFromYAML(levelYAML);
	}

	void LevelSerializer::DeserializeEntities(YAML::Node& entitiesNode, Ref<Level> level)
	{
		for (auto entity : entitiesNode)
		{
			uint64_t uuid = entity["Entity"].as<uint64_t>();

			std::string name;
			auto metadataComponent = entity["MetadataComponent"];
			if (metadataComponent)
			{
				name = metadataComponent["Tag"].as<std::string>();
			}

			RADIANT_INFO("Deserialized entity with ID = {0}, name = {1}", uuid, name);

			Entity deserializedEntity = level->CreateEntityWithUUID(uuid, name);

			auto transformComponent = entity["TransformComponent"];
			if (transformComponent)
			{
				// Entities always have transforms
				auto& tc = deserializedEntity.GetComponent<TransformComponent>();
				tc.Translation = transformComponent["Translation"].as<glm::vec3>();
				tc.Rotation = transformComponent["Rotation"].as<glm::vec3>();
				tc.Scale = transformComponent["Scale"].as<glm::vec3>();
			}

			auto cameraComponent = entity["CameraComponent"];
			if (cameraComponent)
			{
				auto& cc = deserializedEntity.AddComponent<CameraComponent>();

				const auto& cameraNode = cameraComponent["Camera"];

				cc.Camera = LevelCamera();
				auto& camera = cc.Camera;

				if (cameraNode.IsMap())
				{
					if (cameraNode["ProjectionType"])
						camera.SetProjectionType((LevelCamera::ProjectionType)cameraNode["ProjectionType"].as<int>());
					if(cameraNode["AspectRatio"])
						camera.SetAspectRatio(cameraNode["AspectRatio"].as<float>());
					if (cameraNode["PerspectiveFOV"])
						camera.SetDegPerspectiveVerticalFOV(cameraNode["PerspectiveFOV"].as<float>());
					if (cameraNode["PerspectiveNear"])
						camera.SetPerspectiveNearClip(cameraNode["PerspectiveNear"].as<float>());
					if (cameraNode["PerspectiveFar"])
						camera.SetPerspectiveFarClip(cameraNode["PerspectiveFar"].as<float>());
					if (cameraNode["OrthographicSize"])
						camera.SetOrthographicSize(cameraNode["OrthographicSize"].as<float>());
					if (cameraNode["OrthographicNear"])
						camera.SetOrthographicNearClip(cameraNode["OrthographicNear"].as<float>());
					if (cameraNode["OrthographicFar"])
						camera.SetOrthographicFarClip(cameraNode["OrthographicFar"].as<float>());
				}

				cc.Primary = cameraComponent["Primary"].as<bool>();
				cc.FixedAspectRatio = cameraComponent["FixedAspectRatio"].as<bool>();

			}

			auto spriteComponent = entity["SpriteComponent"];
			if (spriteComponent)
			{
				auto& src = deserializedEntity.AddComponent<SpriteComponent>();
				src.Color = spriteComponent["Color"].as<glm::vec4>();

				if (spriteComponent["TextureHandle"])
					src.Texture = spriteComponent["TextureHandle"].as<AssetHandle>();

				if (spriteComponent["TilingFactor"])
					src.TilingFactor = spriteComponent["TilingFactor"].as<float>();
			}

			auto nativeScriptComponent = entity["NativeScriptComponent"];
			if (nativeScriptComponent)
			{
				// TODO: Reflection system?
				//deserializedEntity.AddComponent<NativeScriptComponent>().Bind(nativeScriptComponent["Script Class Name"].as<...>());
			}

			auto rigidbody2DComponent = entity["RigidBody2DComponent"];
			if (rigidbody2DComponent)
			{
				// Add RigidBody at end to properly invoke entt construct signal
				auto rb2d = RigidBody2DComponent();
				rb2d.Type = RigidBody2DBodyTypeFromString(rigidbody2DComponent["BodyType"].as<std::string>());
				rb2d.FixedRotation = rigidbody2DComponent["FixedRotation"].as<bool>();
				deserializedEntity.AddComponent<RigidBody2DComponent>(rb2d);
			}

			auto boxCollider2DComponent = entity["BoxCollider2DComponent"];
			if (boxCollider2DComponent)
			{
				// Add BoxCollider at end to properly invoke entt construct signal
				auto bc2d = BoxCollider2DComponent();
				bc2d.Offset = boxCollider2DComponent["Offset"].as<glm::vec2>();
				bc2d.Size = boxCollider2DComponent["Size"].as<glm::vec2>();
				bc2d.Density = boxCollider2DComponent["Density"].as<float>();
				bc2d.Friction = boxCollider2DComponent["Friction"].as<float>();
				bc2d.Restitution = boxCollider2DComponent["Restitution"].as<float>();
				bc2d.RestitutionThreshold = boxCollider2DComponent["RestitutionThreshold"].as<float>();
				deserializedEntity.AddComponent<BoxCollider2DComponent>(bc2d);
			}
		}

		level->SortEntities();
	}

}