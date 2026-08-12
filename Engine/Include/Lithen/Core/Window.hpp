#pragma once
#include "LithenPCH.hpp"

namespace Lithen {

	namespace {

		constexpr uint32_t WIDTH = 800;
		constexpr uint32_t HEIGHT = 600;

	}

	class Window
	{
	public:
		Window();
		~Window();

		GLFWwindow* GetWindowHandle() const { return m_Window; }

		bool WasResized() const { return m_FrameBufferResized; }
		void ResetResizeFlag() { m_FrameBufferResized = false; }

	private:
		void initWindow();

		static void frameBufferResizeCallback(GLFWwindow* window, int width, int height);

	private:
		GLFWwindow* m_Window = nullptr;

		bool m_FrameBufferResized = false;
	};

}
