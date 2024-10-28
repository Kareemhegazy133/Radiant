#pragma once

#include "rdpch.h"

#include "Base.h"
#include "Events/Event.h"

namespace Radiant {

	struct WindowSpecification
	{
		std::string Title = "Game";
		uint32_t Width = 1280;
		uint32_t Height = 720;
		bool VSync = true;
		std::filesystem::path IconPath;
	};

	// Interface representing a desktop system based Window
	class Window
	{
	public:
		using EventCallbackFn = std::function<void(Event&)>;

		virtual ~Window() {}

		virtual void OnUpdate() = 0;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;

		// Window attributes
		virtual void SetEventCallback(const EventCallbackFn& callback) = 0;
		virtual void SetVSync(bool enabled) = 0;
		virtual bool IsVSync() const = 0;

		virtual void* GetNativeWindow() const = 0;

		static Scope<Window> Create(const WindowSpecification& specification = WindowSpecification());
	};

}