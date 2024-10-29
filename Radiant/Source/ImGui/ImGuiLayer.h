#pragma once

#include "Core/Layer.h"

namespace Radiant {

	struct FontConfiguration
	{
		std::filesystem::path FilePath;
		float Size = 18.0f;
	};

	class ImGuiLayer : public Layer
	{
	public:
		virtual void Begin() = 0;
		virtual void End() = 0;

		void SetFont(const FontConfiguration& config);

		static ImGuiLayer* Create();
		static ImGuiLayer* Create(const FontConfiguration& config);
	};
}