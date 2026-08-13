#include <Lithen.hpp>

#include <imgui.h>

#include <string>
#include <iostream>

int main(int argc, char** argv)
{
	try
	{
		Lithen::Engine engine;
		Lithen::Camera camera{ glm::vec3{ 0.0f, 4.0f, 3.0f }, 45.0f, engine.GetWindowAspectRatio(), 0.1f, 100.0f };
		camera.LookAt(glm::vec3{ 0.0f, 0.0f, 0.0f });

		float lastFrameTime = 0.0f;
		glm::vec2 lastMousePos = Lithen::Input::GetMousePosition();

		while (engine.Update())
		{
			float currentFrameTime = glfwGetTime();
			float deltaTime = currentFrameTime - lastFrameTime;
			lastFrameTime = currentFrameTime;

			camera.SetAspectRatio(engine.GetWindowAspectRatio());

			ImGui::Begin("Sandbox Controls");
			ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
			ImGui::Separator();

			auto& objects = engine.GetGameObjects();
			for (size_t i = 0; i < objects.size(); ++i)
			{
				ImGui::PushID(static_cast<int>(i));

				if (ImGui::TreeNode(("GameObject " + std::to_string(i)).c_str()))
				{
					auto& transform = objects[i].GetTransform();

					ImGui::DragFloat3("Translation", &transform.Translation.x, 0.1f);
					glm::vec3 rotationDeg = glm::degrees(transform.Rotation);
					if (ImGui::DragFloat3("Rotation", &rotationDeg.x, 1.0f))
					{
						transform.Rotation = glm::radians(rotationDeg);
					}
					ImGui::DragFloat3("Scale", &transform.Scale.x, 0.05f);

					ImGui::TreePop();
				}
				ImGui::PopID();
			}
			ImGui::End();

			glm::vec2 currentMousePos = Lithen::Input::GetMousePosition();
			if (!ImGui::GetIO().WantCaptureMouse && Lithen::Input::IsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT))
			{
				float xOffset = currentMousePos.x - lastMousePos.x;
				float yOffset = lastMousePos.y - currentMousePos.y;
				camera.ProcessMouseLook(xOffset, yOffset);
			}
			lastMousePos = currentMousePos;

			camera.OnUpdate(deltaTime);

			engine.Render(camera);
		}

		engine.WaitIdle();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
