#pragma once
#include "LithenPCH.hpp"

#include "Graphics/Model.hpp"
#include "Graphics/Texture.hpp"

namespace Lithen {

	struct Transform
	{
		glm::vec3 Translation{ 0.0f, 0.0f, 0.0f };
		glm::vec3 Rotation{ 0.0f, 0.0f, 0.0f };
		glm::vec3 Scale{ 1.0f, 1.0f, 1.0f };

		glm::mat4 Mat4() const
		{
			glm::mat4 transform = glm::translate(glm::mat4(1.0f), Translation);

			transform = glm::rotate(transform, Rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
			transform = glm::rotate(transform, Rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
			transform = glm::rotate(transform, Rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));

			transform = glm::scale(transform, Scale);
			return transform;
		}
	};

	class GameObject
	{
	public:
		GameObject(std::shared_ptr<Model> model, std::shared_ptr<Texture> texture)
			: m_Model{ std::move(model) }, m_Texture{ std::move(texture) }
		{
		}

		Transform& GetTransform() { return m_Transform; }
		const Transform& GetTransform() const { return m_Transform; }

		std::shared_ptr<Model> GetModel() const { return m_Model; }
		std::shared_ptr<Texture> GetTexture() const { return m_Texture; }

	private:
		Transform m_Transform;
		std::shared_ptr<Model> m_Model;
		std::shared_ptr<Texture> m_Texture;
	};

}
