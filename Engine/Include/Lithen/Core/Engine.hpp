#pragma once
#include "LithenPCH.hpp"

#include "Core/Window.hpp"
#include "Core/Input.hpp"

#include "Graphics/VulkanContext.hpp"
#include "Graphics/Renderer.hpp"
#include "Graphics/Camera.hpp"

namespace Lithen {

	class Engine
	{
	public:
		Engine();
		~Engine();

		bool Update();
		void Render(const Camera& camera);
		void WaitIdle();

		float GetWindowAspectRatio() const;

		std::vector<GameObject>& GetGameObjects() { return m_Renderer.GetGameObjects(); }

	private:
		Window m_Window;
		VKContext m_VKContext;
		Renderer m_Renderer;
	};

}
