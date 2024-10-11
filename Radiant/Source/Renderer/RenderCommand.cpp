#include "rdpch.h"
#include "RenderCommand.h"

namespace Radiant {

	Scope<RendererAPI> RenderCommand::s_RendererAPI = RendererAPI::Create();

}