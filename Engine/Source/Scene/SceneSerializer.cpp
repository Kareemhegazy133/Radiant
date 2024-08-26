#include "Enginepch.h"

#include "SceneSerializer.h"

#include "GameObject.h"
#include "Components.h"

#include <fstream>

#include <yaml-cpp/yaml.h>

namespace YAML {

	template<>
	struct convert<sf::Vector2f>
	{
		static Node encode(const sf::Vector2f& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, sf::Vector2f& rhs)
		{
			if (!node.IsSequence() || node.size() != 2)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			return true;
		}
	};

	template<>
	struct convert<Engine::UUID>
	{
		static Node encode(const Engine::UUID& uuid)
		{
			Node node;
			node.push_back((uint64_t)uuid);
			return node;
		}

		static bool decode(const Node& node, Engine::UUID& uuid)
		{
			uuid = node.as<uint64_t>();
			return true;
		}
	};
}

namespace Engine {

	YAML::Emitter& operator<<(YAML::Emitter& out, const sf::Vector2f& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << YAML::EndSeq;
		return out;
	}

	static std::string RigidBody2DBodyTypeToString(Rigidbody2DComponent::BodyType bodyType)
	{
		switch (bodyType)
		{
		case Rigidbody2DComponent::BodyType::Static:    return "Static";
		case Rigidbody2DComponent::BodyType::Dynamic:   return "Dynamic";
		case Rigidbody2DComponent::BodyType::Kinematic: return "Kinematic";
		}

		ENGINE_ASSERT(false, "Unknown body type");
		return {};
	}

	static Rigidbody2DComponent::BodyType RigidBody2DBodyTypeFromString(const std::string& bodyTypeString)
	{
		if (bodyTypeString == "Static")    return Rigidbody2DComponent::BodyType::Static;
		if (bodyTypeString == "Dynamic")   return Rigidbody2DComponent::BodyType::Dynamic;
		if (bodyTypeString == "Kinematic") return Rigidbody2DComponent::BodyType::Kinematic;

		ENGINE_ASSERT(false, "Unknown body type");
		return Rigidbody2DComponent::BodyType::Static;
	}

	SceneSerializer::SceneSerializer(Scene* scene)
		: m_Scene(scene)
	{

	}

	static void SerializeGameObject(YAML::Emitter& out, GameObject gameObject)
	{
		out << YAML::BeginMap; // Game Object
		out << YAML::Key << "Game Object" << YAML::Value << gameObject.GetUUID();

		if (gameObject.HasComponent<MetadataComponent>())
		{
			out << YAML::Key << "MetadataComponent";
			out << YAML::BeginMap; // MetadataComponent

			auto& tag = gameObject.GetComponent<MetadataComponent>().Tag;
			out << YAML::Key << "Tag" << YAML::Value << tag;

			out << YAML::EndMap; // MetadataComponent
		}

		if (gameObject.HasComponent<TransformComponent>())
		{
			out << YAML::Key << "TransformComponent";
			out << YAML::BeginMap; // TransformComponent

			auto& transform = gameObject.GetComponent<TransformComponent>();
			out << YAML::Key << "Translation" << YAML::Value << transform.GetPosition();
			out << YAML::Key << "Rotation" << YAML::Value << transform.GetRotation();
			out << YAML::Key << "Scale" << YAML::Value << transform.GetScale();

			out << YAML::EndMap; // TransformComponent
		}

		if (gameObject.HasComponent<SpriteComponent>())
		{
			out << YAML::Key << "SpriteComponent";
			out << YAML::BeginMap; // SpriteComponent

			auto& sprite = gameObject.GetComponent<SpriteComponent>();
			out << YAML::Key << "Texture Identifier" << YAML::Value << sprite.GetTextureIdentifier();

			out << YAML::EndMap; // SpriteComponent
		}

		if (gameObject.HasComponent<Rigidbody2DComponent>())
		{
			out << YAML::Key << "Rigidbody2DComponent";
			out << YAML::BeginMap; // Rigidbody2DComponent

			auto& rb2d = gameObject.GetComponent<Rigidbody2DComponent>();
			out << YAML::Key << "BodyType" << YAML::Value << RigidBody2DBodyTypeToString(rb2d.Type);
			out << YAML::Key << "FixedRotation" << YAML::Value << rb2d.FixedRotation;

			out << YAML::EndMap; // Rigidbody2DComponent
		}

		if (gameObject.HasComponent<BoxCollider2DComponent>())
		{
			out << YAML::Key << "BoxCollider2DComponent";
			out << YAML::BeginMap; // BoxCollider2DComponent

			auto& bc2d = gameObject.GetComponent<BoxCollider2DComponent>();
			out << YAML::Key << "Offset" << YAML::Value << bc2d.Offset;
			out << YAML::Key << "Density" << YAML::Value << bc2d.Density;
			out << YAML::Key << "Friction" << YAML::Value << bc2d.Friction;
			out << YAML::Key << "Restitution" << YAML::Value << bc2d.Restitution;
			out << YAML::Key << "RestitutionThreshold" << YAML::Value << bc2d.RestitutionThreshold;

			out << YAML::EndMap; // BoxCollider2DComponent
		}

		if (gameObject.HasComponent<CharacterComponent>())
		{
			out << YAML::Key << "CharacterComponent";
			out << YAML::BeginMap; // CharacterComponent

			auto& character = gameObject.GetComponent<CharacterComponent>();
			out << YAML::Key << "Level" << YAML::Value << character.Level;
			out << YAML::Key << "Coins" << YAML::Value << character.Coins;
			out << YAML::Key << "Diamonds" << YAML::Value << character.Diamonds;
			out << YAML::Key << "Current Health" << YAML::Value << character.CurrentHealth;
			out << YAML::Key << "Current Stamina" << YAML::Value << character.CurrentStamina;
			out << YAML::Key << "Speed" << YAML::Value << character.Speed;
			out << YAML::Key << "Direction" << YAML::Value << character.Direction;

			out << YAML::EndMap; // CharacterComponent
		}

		if (gameObject.HasComponent<AttributesComponent>())
		{
			out << YAML::Key << "AttributesComponent";
			out << YAML::BeginMap; // AttributesComponent

			auto& attributes = gameObject.GetComponent<AttributesComponent>();
			out << YAML::Key << "Total Attribute Points Spent" << YAML::Value << attributes.GetTotalAttributePointsSpent();
			out << YAML::Key << "Attribute Points Available" << YAML::Value << attributes.GetAttributePointsAvailable();
			// TODO : Show each attribute

			out << YAML::EndMap; // AttributesComponent
		}

		out << YAML::EndMap; // GameObject
	}

	void SceneSerializer::Serialize(const std::string& filepath)
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Scene" << YAML::Value << "Untitled";
		out << YAML::Key << "Game Objects" << YAML::Value << YAML::BeginSeq;
		for (auto gameObjectID : m_Scene->m_Registry.view<entt::entity>())
		{
			GameObject gameObject = { gameObjectID, m_Scene };
			if (!gameObject)
				return;

			SerializeGameObject(out, gameObject);
		}
		out << YAML::EndSeq;
		out << YAML::EndMap;

		std::ofstream fout(filepath);
		fout << out.c_str();
	}

	bool SceneSerializer::Deserialize(const std::string& filepath)
	{
		YAML::Node data;
		try
		{
			data = YAML::LoadFile(filepath);
		}
		catch (YAML::ParserException e)
		{
			ENGINE_ERROR("Failed to load .sav file '{0}'\n     {1}", filepath, e.what());
			return false;
		}

		if (!data["Scene"])
			return false;

		std::string sceneName = data["Scene"].as<std::string>();
		ENGINE_TRACE("Deserializing scene '{0}'", sceneName);
		
		auto gameObjects = data["Game Objects"];
		if (gameObjects)
		{
			for (auto gameObject : gameObjects)
			{
				uint64_t uuid = gameObject["Game Object"].as<uint64_t>();

				std::string name;
				auto metadataComponent = gameObject["MetadataComponent"];
				if (metadataComponent)
					name = metadataComponent["Tag"].as<std::string>();

				ENGINE_TRACE("Deserialized gameObject with ID = {0}, name = {1}", uuid, name);

				GameObject deserializedGameObject = m_Scene->CreateGameObjectWithUUID(uuid, name);

				auto transformComponent = gameObject["TransformComponent"];
				if (transformComponent)
				{
					// GameObjects always have transforms
					auto& transform = deserializedGameObject.GetComponent<TransformComponent>();
					transform.SetPosition(transformComponent["Translation"].as<sf::Vector2f>());
					transform.SetRotation(transformComponent["Rotation"].as<float>());
					transform.SetScale(transformComponent["Scale"].as<sf::Vector2f>());
				}

				auto spriteComponent = gameObject["SpriteComponent"];
				if (spriteComponent)
				{
					auto& sprite = deserializedGameObject.AddComponent<SpriteComponent>();
					sprite.SetTextureIdentifier(spriteComponent["Texture Identifier"].as<std::string>());
				}

				auto rigidbody2DComponent = gameObject["Rigidbody2DComponent"];
				if (rigidbody2DComponent)
				{
					auto& rb2d = deserializedGameObject.AddComponent<Rigidbody2DComponent>();
					rb2d.Type = RigidBody2DBodyTypeFromString(rigidbody2DComponent["BodyType"].as<std::string>());
					rb2d.FixedRotation = rigidbody2DComponent["FixedRotation"].as<bool>();
				}

				auto boxCollider2DComponent = gameObject["BoxCollider2DComponent"];
				if (boxCollider2DComponent)
				{
					auto& bc2d = deserializedGameObject.AddComponent<BoxCollider2DComponent>();
					bc2d.Offset = boxCollider2DComponent["Offset"].as<sf::Vector2f>();
					bc2d.Density = boxCollider2DComponent["Density"].as<float>();
					bc2d.Friction = boxCollider2DComponent["Friction"].as<float>();
					bc2d.Restitution = boxCollider2DComponent["Restitution"].as<float>();
					bc2d.RestitutionThreshold = boxCollider2DComponent["RestitutionThreshold"].as<float>();
				}

				auto characterComponent = gameObject["CharacterComponent"];
				if (characterComponent)
				{
					auto& character = deserializedGameObject.AddComponent<CharacterComponent>();
					character.Level = characterComponent["Level"].as<int>();
					character.Coins = characterComponent["Coins"].as<int>();
					character.Diamonds = characterComponent["Diamonds"].as<int>();
					character.CurrentHealth = characterComponent["Current Health"].as<float>();
					character.CurrentStamina = characterComponent["Current Stamina"].as<float>();
					character.Speed = characterComponent["Speed"].as<float>();
					character.Direction = characterComponent["Direction"].as<sf::Vector2f>();
				}

				auto attributesComponent = gameObject["AttributesComponent"];
				if (attributesComponent)
				{
					auto& attributes = deserializedGameObject.AddComponent<AttributesComponent>();
					// TODO: Allocate total attribute points spent in each attribute
					attributes.AddAttributePoints(attributesComponent["Attribute Points Available"].as<int>());
				}
			}
		}

		return true;
	}
}