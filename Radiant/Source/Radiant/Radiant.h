#pragma once

// For use by Game applications

#include "Radiant/Core/Base.h"

#include "Radiant/Core/GameApplication.h"
#include "Radiant/Core/Layer.h"
#include "Radiant/Core/Log.h"

#include "Radiant/Core/Timestep.h"

#include "Radiant/Core/Input.h"
#include "Radiant/Core/MouseCodes.h"
#include "Radiant/Core/KeyCodes.h"

#include "Radiant/Events/Event.h"
#include "Radiant/Events/MouseEvent.h"
#include "Radiant/Events/KeyEvent.h"

#include <imgui/imgui.h>

#include "Radiant/Physics/Physics2D.h"

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
#include "Radiant/Renderer/Framebuffer.h"
#include "Radiant/Renderer/Texture.h"
#include "Radiant/Renderer/SubTexture2D.h"
#include "Radiant/Renderer/VertexArray.h"

#include "Radiant/Renderer/SceneCamera.h"

using namespace Radiant;