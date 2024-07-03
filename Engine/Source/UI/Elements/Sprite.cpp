#include "Enginepch.h"

#include "Sprite.h"

namespace Engine {

	Sprite::Sprite(const sf::Texture& texture, const sf::Vector2f& position)
		: m_Sprite(texture)
	{
		SetPosition(position);
	}

	void Sprite::SetTexture(const sf::Texture& texture)
	{
		m_Sprite.setTexture(texture);
	}

	void Sprite::SetPosition(const sf::Vector2f& position)
	{
		m_Sprite.setPosition(position);
	}

	void Sprite::SetScale(const sf::Vector2f& factors)
	{
		m_Sprite.setScale(factors);
	}

	void Sprite::SetScale(float factorX, float factorY)
	{
		m_Sprite.setScale(factorX, factorY);
	}
}
