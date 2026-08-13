#pragma once
#include "LithenPCH.hpp"

#include "Core/Input.hpp"

namespace Lithen {

	class Camera
	{
	public:
		Camera(glm::vec3 position, float fov, float aspect, float nearClip, float farClip);

		void OnUpdate(float deltaTime);
		void ProcessMouseLook(float xOffset, float yOffset);
		void LookAt(glm::vec3 target);

		void SetAspectRatio(float aspect) { m_AspectRatio = aspect; }

		glm::mat4 GetViewMatrix() const;
		glm::mat4 GetProjectionMatrix() const;

	private:
		void updateVectors();

	private:
		glm::vec3 m_Position;
		glm::vec3 m_Front;
		glm::vec3 m_Up;
		glm::vec3 m_Right;
		glm::vec3 m_WorldUp;

		float m_Yaw;
		float m_Pitch;

		float m_MovementSpeed;
		float m_MouseSensitivity;

		float m_Fov;
		float m_AspectRatio;
		float m_NearClip;
		float m_FarClip;
	};

}
