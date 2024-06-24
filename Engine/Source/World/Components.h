#pragma once

// Single header file for all Components

#include "World/Components/Component.h"

// Common Components

// TODO: See if operator overloading is possible for sfml default functions
#include "World/Components/MetadataComponent.h"
#include "World/Components/TransformComponent.h"
#include "World/Components/SpriteComponent.h"
#include "World/Components/AnimationComponent.h"

// Gameplay Entities Components
#include "World/Components/RigidBody2DComponent.h"
#include "World/Components/BoxCollider2DComponent.h"
#include "World/Components/AttributesComponent.h"
#include "World/Components/AbilitiesComponent.h"

// Specific Gameplay Entities' based Components
#include "World/Components/CharacterComponent.h"
#include "World/Components/AbilityComponent.h"

// UI Entities Components
#include "World/Components/TextComponent.h"
#include "World/Components/ButtonComponent.h"
