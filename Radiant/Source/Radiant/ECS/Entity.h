#pragma once

#include <entt.hpp>

#include "Components.h"

namespace Radiant {

	class Level;

	/**
	 * Value handle to an entity: {entt handle, Level*}, 16 bytes, copied freely.
	 * Owns nothing — the Level owns all entity and component storage.
	 *
	 * Handles are TRANSIENT: they dangle once the entity (or its Level) is
	 * destroyed, and are never serialized — the persistent identity is the UUID
	 * (GetUUID / Level::GetEntityByUUID). Check IsValid() / operator bool before
	 * using a handle you did not just obtain from the Level. Main-thread only.
	 *
	 * Component accessors assert on misuse in Debug/Release (asserts compile out
	 * in Dist): GetComponent on a missing component, AddComponent on a duplicate,
	 * and every accessor on an invalid handle.
	 */
	class Entity
	{
	public:
		Entity() = default;
		Entity(entt::entity handle, Level* level)
			: m_EntityHandle(handle), m_Level(level) {}
		Entity(const Entity& other) = default;

		~Entity() = default;

		/**
		 * Constructs T in place from args and returns a reference into the component
		 * pool — invalidated by later pool mutations, so do not cache it. Asserts if
		 * the entity already has T. Adding a physics component fires its entt
		 * construct signal (creates the Box2D body/fixture) before returning.
		 */
		template<typename T, typename... Args>
		T& AddComponent(Args&&... args);

		/** As AddComponent, but replaces an existing T instead of asserting. */
		template<typename T, typename... Args>
		T& AddOrReplaceComponent(Args&&... args);

		/**
		 * Returns a reference to the entity's T; asserts if the component is missing.
		 * The reference is invalidated by later pool mutations — do not cache it.
		 */
		template<typename T>
		T& GetComponent();

		template<typename T>
		const T& GetComponent() const;

		// returns nullptr if entity does not have the requested component type
		template<typename T>
		T* TryGetComponent();

		// returns nullptr if entity does not have the requested component type
		template<typename T>
		const T* TryGetComponent() const;

		/** True if the entity has all of the listed component types. */
		template<typename... T>
		bool HasComponent();

		template<typename... T>
		bool HasComponent() const;

		/**
		 * Removes T immediately; asserts if missing. Removal fires the component's
		 * entt on_destroy signal (for physics components this destroys the body).
		 */
		template<typename T>
		void RemoveComponent();

		/** As RemoveComponent, but a no-op when the component is absent. */
		template<typename T>
		void RemoveComponentIfExists();

		/**
		 * Moves this entity discontinuously — forwarders to Level::Teleport
		 * (see it for the full contract: ECS write + snapshot reset + explicit
		 * physics push); the only logic here is warning on a level-less handle,
		 * which cannot reach the Level to be warned about. rotationZ is
		 * radians; the overload without it keeps the current rotation.
		 */
		void Teleport(const glm::vec3& translation, float rotationZ);
		void Teleport(const glm::vec3& translation);

		// The non-const overload hands out a mutable reference to the shared static
		// fallback when metadata is missing — do not write through it in that case
		std::string& Name() { return HasComponent<MetadataComponent>() ? GetComponent<MetadataComponent>().Tag : NoName; }
		const std::string& Name() const { return HasComponent<MetadataComponent>() ? GetComponent<MetadataComponent>().Tag : NoName; }

		operator bool() const;
		operator entt::entity() const { return m_EntityHandle; }
		operator uint32_t() const { return (uint32_t)m_EntityHandle; }

		/** The entity's persistent identity — stable across save/load, unlike this handle. */
		UUID GetUUID() { return GetComponent<MetadataComponent>().ID; }

		/** True when this handle refers to a live entity in a live Level. */
		bool IsValid() const;

		bool operator==(const Entity& other) const
		{
			return m_EntityHandle == other.m_EntityHandle && m_Level == other.m_Level;
		}

		bool operator!=(const Entity& other) const
		{
			return !(*this == other);
		}

	private:
		entt::entity m_EntityHandle{ entt::null };
		Level* m_Level = nullptr;

		inline static std::string NoName = "Unnamed";

		friend class Level;
	};
}