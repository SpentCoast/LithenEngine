#pragma once
#include "LithenPCH.hpp"

#include "Core/Window.hpp"

#include "Graphics/VulkanContext.hpp"
#include "Graphics/Renderer.hpp"

namespace Lithen {

	class Engine
	{
	public:
		Engine();
		~Engine();

		void Run();

	private:
		Window m_Window;
		VKContext m_VKContext;
		Renderer m_Renderer;
	};

}
