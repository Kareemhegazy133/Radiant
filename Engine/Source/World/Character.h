#pragma once

#include "Entity.h"

namespace Engine {

    class Character : public Entity {
    public:
        Character(const std::string& entityName, const std::string& textureIdentifier, const sf::Vector2f& position);

        virtual ~Character();

        virtual void OnUpdate(Timestep ts) = 0;

        virtual void OnCollisionBegin(const Entity& other) override;
        virtual void OnCollisionEnd(const Entity& other) override;

    protected:
        std::string m_textureIdentifier;

        sf::Vector2f m_Position;
    };

}
