#pragma once
#include "LithenPCH.hpp"

#include "Graphics/VulkanContext.hpp"
#include "Graphics/Buffer.hpp"

#include <tiny_obj_loader.h>

namespace Lithen {

	struct Vertex
	{
		glm::vec3 Position;
		glm::vec3 Color;
		glm::vec2 TextureCoordinate;

		static vk::VertexInputBindingDescription GetBindingDescription()
		{
			return vk::VertexInputBindingDescription{ 0, sizeof(Vertex), vk::VertexInputRate::eVertex };
		}

		static std::array<vk::VertexInputAttributeDescription, 3> GetAttributeDescriptions()
		{
			return {
				vk::VertexInputAttributeDescription{ 0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, Position) },
				vk::VertexInputAttributeDescription{ 1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, Color) },
				vk::VertexInputAttributeDescription{ 2, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, TextureCoordinate) }
			};
		}

		bool operator==(const Vertex& other) const
		{
			return Position == other.Position && Color == other.Color && TextureCoordinate == other.TextureCoordinate;
		}
	};

}

namespace std {

	template<> struct hash<Lithen::Vertex>
	{
		size_t operator()(const Lithen::Vertex& vertex) const noexcept
		{
			return ((hash<glm::vec3>()(vertex.Position) ^ (hash<glm::vec3>()(vertex.Color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.TextureCoordinate) << 1);
		}
	};

}

namespace Lithen {

	class Model
	{
	public:
		Model(const VKContext& context, const std::string& filepath);
		~Model() = default;

		void Bind(vk::raii::CommandBuffer& commandBuffer) const;
		void Draw(vk::raii::CommandBuffer& commandBuffer) const;

	private:
		void loadModel(const std::string& filepath);
		void createVertexBuffers(const std::vector<Vertex>& vertices);
		void createIndexBuffers(const std::vector<uint32_t>& indieces);

	private:
		const VKContext& m_Context;

		std::unique_ptr<Buffer> m_VertexBuffer;
		std::unique_ptr<Buffer> m_IndexBuffer;
		uint32_t m_IndexCount = 0;
	};

}
