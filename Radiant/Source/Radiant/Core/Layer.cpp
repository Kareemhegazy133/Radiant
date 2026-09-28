#include "Radiant/rdpch.h"
#include "Layer.h"

namespace Radiant {

	Layer::Layer(const std::string& debugName /*= "Layer"*/)
		: m_DebugName(debugName)
	{
	}

	Layer::~Layer()
	{
	}
}