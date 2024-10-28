#pragma once

// For use by Game applications

#include "Core/Base.h"

#include "Core/GameApplication.h"
#include "Core/Layer.h"
#include "Core/GameState.h"
#include "Core/Log.h"

#include "Core/Timestep.h"

#include "Core/Input.h"
#include "Core/MouseCodes.h"
#include "Core/KeyCodes.h"

#include "Events/Event.h"
#include "Events/MouseEvent.h"
#include "Events/KeyEvent.h"

#include "imgui/imgui.h"

#include "Physics/Physics2D.h"

#include "ECS/Level.h"
#include "ECS/Components.h"
#include "ECS/Entity.h"
#include "ECS/ScriptableEntity.h"
#include "ECS/LevelSerializer.h"

#include "Asset/AssetManager.h"

// ---Renderer------------------------

#include "Renderer/Renderer.h"
#include "Renderer/Renderer2D.h"
#include "Renderer/RenderCommand.h"

#include "Renderer/Buffer.h"
#include "Renderer/Shader.h"
#include "Renderer/Framebuffer.h"
#include "Renderer/Texture.h"
#include "Renderer/SubTexture2D.h"
#include "Renderer/VertexArray.h"

#include "Renderer/SceneCamera.h"
