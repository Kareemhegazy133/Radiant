#pragma once

#include "Entity.h"

namespace Engine {

    class Character : public Entity {
    public:
        Character(const std::string& entityName);
        //Character(const std::string& entityName, const std::string& textureIdentifier, const sf::Vector2f& position);

        virtual ~Character();

        virtual void OnUpdate(Timestep ts) = 0;
    };

}
