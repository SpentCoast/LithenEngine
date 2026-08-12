#include "Core/Engine.hpp"

namespace Lithen {

	Engine::Engine()
		: m_VKContext{ m_Window }, m_Renderer{ m_Window, m_VKContext }
	{
	}

	Engine::~Engine()
	{
	}

	void Engine::Run()
	{
		while (!glfwWindowShouldClose(m_Window.GetWindowHandle()))
		{
			glfwPollEvents();
			m_Renderer.DrawFrame();
		}

		m_VKContext.GetDevice().waitIdle();
	}

}
