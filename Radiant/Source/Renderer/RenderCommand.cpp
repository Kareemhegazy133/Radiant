#include "rdpch.h"
#include "Renderer/RenderCommand.h"

namespace Radiant {

	Scope<RendererAPI> RenderCommand::s_RendererAPI = RendererAPI::Create();

}