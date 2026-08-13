#pragma once
#include "LithenPCH.hpp"

namespace Lithen {

	class Input
	{
	public:
		static void Init(void* windowHandle);

		static bool IsKeyPressed(int keycode);
		static bool IsMouseButtonPressed(int button);
		static glm::vec2 GetMousePosition();

	private:
		static void* s_Window;
	};

}
