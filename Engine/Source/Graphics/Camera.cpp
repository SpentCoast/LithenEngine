#include "Graphics/Camera.hpp"

namespace Lithen {

	Camera::Camera(glm::vec3 position, float fov, float aspect, float nearClip, float farClip)
		: m_Position{ position }, m_Yaw{ 0.0f }, m_Pitch{ 0.0f },
		m_MovementSpeed{ 2.0f }, m_MouseSensitivity{ 0.05f },
		m_Fov{ fov }, m_AspectRatio{ aspect }, m_NearClip{ nearClip }, m_FarClip{ farClip }
	{
		m_WorldUp = glm::vec3(0.0f, 0.0f, 1.0f);
		updateVectors();
	}

	void Camera::OnUpdate(float deltaTime)
	{
		float velocity = m_MovementSpeed * deltaTime;

		if (Input::IsKeyPressed(GLFW_KEY_W)) m_Position += m_Front * velocity;
		if (Input::IsKeyPressed(GLFW_KEY_S)) m_Position -= m_Front * velocity;
		if (Input::IsKeyPressed(GLFW_KEY_A)) m_Position -= m_Right * velocity;
		if (Input::IsKeyPressed(GLFW_KEY_D)) m_Position += m_Right * velocity;
		if (Input::IsKeyPressed(GLFW_KEY_E)) m_Position += m_Up * velocity;
		if (Input::IsKeyPressed(GLFW_KEY_Q)) m_Position -= m_Up * velocity;
	}

	void Camera::ProcessMouseLook(float xOffset, float yOffset)
	{
		xOffset *= m_MouseSensitivity;
		yOffset *= m_MouseSensitivity;

		m_Yaw += xOffset;
		m_Pitch -= yOffset;

		if (m_Pitch > 89.0f) m_Pitch = 89.0f;
		if (m_Pitch < -89.0f) m_Pitch = -89.0f;

		updateVectors();
	}

	void Camera::LookAt(glm::vec3 target)
	{
		glm::vec3 direction = glm::normalize(target - m_Position);
		m_Pitch = glm::degrees(asin(direction.z));
		m_Yaw = glm::degrees(atan2(direction.y, direction.x));

		updateVectors();
	}

	glm::mat4 Camera::GetViewMatrix() const
	{
		return glm::lookAt(m_Position, m_Position + m_Front, m_Up);
	}

	glm::mat4 Camera::GetProjectionMatrix() const
	{
		glm::mat4 proj = glm::perspective(glm::radians(m_Fov), m_AspectRatio, m_NearClip, m_FarClip);
		proj[1][1] *= -1;
		return proj;
	}

	void Camera::updateVectors()
	{
		glm::vec3 front{};
		front.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
		front.y = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
		front.z = sin(glm::radians(m_Pitch));

		m_Front = glm::normalize(front);
		m_Right = glm::normalize(glm::cross(m_Front, m_WorldUp));
		m_Up = glm::normalize(glm::cross(m_Right, m_Front));
	}

}
