#pragma once

#include "entt.hpp"

#include <SFML/System/String.hpp>
#include "Core/Timestep.h"

namespace Engine {

	class Entity;

	class Scene
	{
	public:
		Scene();
		~Scene();

		Entity CreateEntity(const sf::String& name = sf::String());
		void OnUpdate(Timestep ts);

	private:
		entt::registry m_Registry;

		friend class Entity;
	};
}