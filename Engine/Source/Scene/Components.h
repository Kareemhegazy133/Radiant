#pragma once

// Single header file for all Components

#include "Scene/Components/Component.h"

// Common Components

// TODO: See if operator overloading is possible for sfml default functions
#include "Scene/Components/MetadataComponent.h"
#include "Scene/Components/TransformComponent.h"
#include "Scene/Components/SpriteComponent.h"
#include "Scene/Components/AnimationComponent.h"

// Gameplay Entities Components
#include "Scene/Components/RigidBody2DComponent.h"
#include "Scene/Components/BoxCollider2DComponent.h"
#include "Scene/Components/AttributesComponent.h"
#include "Scene/Components/AbilitiesComponent.h"

// Specific Gameplay Entities' based Components
#include "Scene/Components/CharacterComponent.h"
#include "Scene/Components/AbilityComponent.h"
