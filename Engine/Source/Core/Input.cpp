#include "Core/Input.hpp"

namespace Lithen {

	void* Input::s_Window = nullptr;

	void Input::Init(void* windowHandle)
	{
		s_Window = windowHandle;
	}

	bool Input::IsKeyPressed(int keycode)
	{
		auto state = glfwGetKey(static_cast<GLFWwindow*>(s_Window), keycode);
		return state == GLFW_PRESS || state == GLFW_REPEAT;
	}

	bool Input::IsMouseButtonPressed(int button)
	{
		auto state = glfwGetMouseButton(static_cast<GLFWwindow*>(s_Window), button);
		return state == GLFW_PRESS;
	}

	glm::vec2 Input::GetMousePosition()
	{
		double xpos, ypos;
		glfwGetCursorPos(static_cast<GLFWwindow*>(s_Window), &xpos, &ypos);
		return { static_cast<float>(xpos), static_cast<float>(ypos) };
	}

}
