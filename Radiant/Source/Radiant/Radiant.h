#pragma once

// Umbrella header for game applications — the single include a game needs.
// Engine code never includes this; it uses targeted includes and the PCH.

#include "Radiant/Core/Base.h"

#include "Radiant/Core/GameApplication.h"
#include "Radiant/Core/Layer.h"
#include "Radiant/Core/Log.h"

#include "Radiant/Core/Timestep.h"
#include "Radiant/Core/Time.h"

#include "Radiant/Core/Input.h"
#include "Radiant/Core/MouseCodes.h"
#include "Radiant/Core/KeyCodes.h"

#include "Radiant/Events/Event.h"
#include "Radiant/Events/MouseEvent.h"
#include "Radiant/Events/KeyEvent.h"

#include <imgui/imgui.h>

// No physics include: PhysicsWorld2D is Level-internal (game code drives
// physics through components; Level.h forward-declares the world) — keeping
// it out of the umbrella keeps Box2D headers out of every game TU (RAD-27)

#include "Radiant/ECS/Level.h"
#include "Radiant/ECS/Components.h"
#include "Radiant/ECS/Entity.h"
#include "Radiant/ECS/ScriptableEntity.h"
#include "Radiant/ECS/LevelSerializer.h"

#include "Radiant/Asset/AssetManager.h"

// ---Renderer------------------------

#include "Radiant/Renderer/Renderer.h"
#include "Radiant/Renderer/Renderer2D.h"
#include "Radiant/Renderer/RenderCommand.h"

#include "Radiant/Renderer/Buffer.h"
#include "Radiant/Renderer/Shader.h"
#include "Radiant/Renderer/FrameBuffer.h"
#include "Radiant/Renderer/Texture.h"
#include "Radiant/Renderer/SubTexture2D.h"
#include "Radiant/Renderer/VertexArray.h"

#include "Radiant/Renderer/SceneCamera.h"

// Note: this header deliberately does NOT inject `using namespace Radiant` —
// an engine header must never force namespace pollution on its consumers.
// Game code may opt in with its own using-directive.