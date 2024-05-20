#pragma once

#include "Enginepch.h"
#include "WindowsWindow.h"

#include <SFML/Window/Event.hpp>

#include "Events/ApplicationEvent.h"
#include "Events/MouseEvent.h"
#include "Events/KeyEvent.h"

namespace Engine {

    WindowsWindow::WindowsWindow(const WindowProps& props)
    {
        Init(props);
    }

    WindowsWindow::~WindowsWindow()
    {
        Shutdown();
    }

    void WindowsWindow::Init(const WindowProps& props)
    {
        m_Data.Title = props.Title;
        m_Data.Width = props.Width;
        m_Data.Height = props.Height;

        ENGINE_INFO("Creating window {0} ({1}, {2})", props.Title, props.Width, props.Height);

        m_Window = new sf::RenderWindow(sf::VideoMode((int)props.Width, (int)props.Height), m_Data.Title);

        SetVSync(true);
    }

    void WindowsWindow::Shutdown()
    {
        m_Window->close();
    }

    void WindowsWindow::Display()
    {
        m_Window->display();
    }

    bool WindowsWindow::IsRunning() const
    {
       return m_Window->isOpen();
    }

    void WindowsWindow::OnUpdate()
    {
        sf::Event event;
        while (m_Window->pollEvent(event))
        {
            switch (event.type)
            {
                case sf::Event::Closed:
                {
                    WindowCloseEvent closeEvent;
                    m_Data.EventCallback(closeEvent);
                    break;
                }
                case sf::Event::Resized:
                {
                    WindowResizeEvent resizeEvent(event.size.width, event.size.height);
                    m_Data.Width = event.size.width;
                    m_Data.Height = event.size.height;
                    m_Data.EventCallback(resizeEvent);
                    break;
                }
                case sf::Event::KeyPressed:
                {
                    KeyPressedEvent keyEvent(event.key.code, 0);
                    m_Data.EventCallback(keyEvent);
                    break;
                }
                case sf::Event::KeyReleased:
                {
                    KeyReleasedEvent keyEvent(event.key.code);
                    m_Data.EventCallback(keyEvent);
                    break;
                }
                case sf::Event::TextEntered:
                {
                    KeyTypedEvent keyTypedEvent(event.text.unicode);
                    m_Data.EventCallback(keyTypedEvent);
                    break;
                }
                case sf::Event::MouseButtonPressed:
                {
                    MouseButtonPressedEvent mouseButtonPressedEvent(event.mouseButton.button);
                    m_Data.EventCallback(mouseButtonPressedEvent);
                    break;
                }
                case sf::Event::MouseButtonReleased:
                {
                    MouseButtonReleasedEvent mouseButtonReleasedEvent(event.mouseButton.button);
                    m_Data.EventCallback(mouseButtonReleasedEvent);
                    break;
                }
                case sf::Event::MouseMoved:
                {
                    MouseMovedEvent mouseMovedEvent(event.mouseMove.x, event.mouseMove.y);
                    m_Data.EventCallback(mouseMovedEvent);
                    break;
                }
                case sf::Event::MouseWheelScrolled:
                {
                    MouseScrolledEvent mouseScrolledEvent(event.mouseWheelScroll.delta, event.mouseWheelScroll.delta);
                    m_Data.EventCallback(mouseScrolledEvent);
                    break;
                }
            }
        }

        m_Window->clear(sf::Color::Black);
    }

    void WindowsWindow::SetVSync(bool enabled)
    {
        m_Window->setVerticalSyncEnabled(enabled);
        m_Data.VSync = enabled;
    }

    bool WindowsWindow::IsVSync() const
    {
        return m_Data.VSync;
    }

}
