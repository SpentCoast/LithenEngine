#include "Core/Engine.hpp"

namespace Lithen {

	Engine::Engine()
		: m_VKContext{ m_Window }, m_Renderer{ m_Window, m_VKContext }
	{
		Input::Init(m_Window.GetWindowHandle());
	}

	Engine::~Engine()
	{
	}

	bool Engine::Update()
	{
		glfwPollEvents();
		
		if (glfwWindowShouldClose(m_Window.GetWindowHandle())) return false;

		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		return true;
	}

	void Engine::Render(const Camera& camera)
	{
		ImGui::Render();
		m_Renderer.DrawFrame(camera);
	}

	void Engine::WaitIdle()
	{
		m_VKContext.GetDevice().waitIdle();
	}

	float Engine::GetWindowAspectRatio() const
	{
		int width, height;
		glfwGetFramebufferSize(m_Window.GetWindowHandle(), &width, &height);

		if (height == 0)
		{
			return 1.0f;
		}

		return static_cast<float>(width) / static_cast<float>(height);
	}

}
