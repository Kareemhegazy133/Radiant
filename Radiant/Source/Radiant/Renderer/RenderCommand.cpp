#include "Radiant/rdpch.h"
#include "RenderCommand.h"

namespace Radiant {

	// Runs at static initialization, before main() and before any context or
	// logging exists — safe only because backend constructors do no graphics
	// work; all GL calls wait for RenderCommand::Init()
	Scope<RendererAPI> RenderCommand::s_RendererAPI = RendererAPI::Create();

}