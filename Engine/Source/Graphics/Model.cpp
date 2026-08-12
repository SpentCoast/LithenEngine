#include "Graphics/Model.hpp"

namespace Lithen {

	Model::Model(const VKContext& context, const std::string& filepath)
		: m_Context{ context }
	{
		loadModel(filepath);
	}

	void Model::Bind(vk::raii::CommandBuffer& commandBuffer) const
	{
		commandBuffer.bindVertexBuffers(0, *m_VertexBuffer->GetHandle(), { 0 });
		commandBuffer.bindIndexBuffer(*m_IndexBuffer->GetHandle(), 0, vk::IndexType::eUint32);
	}

	void Model::Draw(vk::raii::CommandBuffer& commandBuffer) const
	{
		commandBuffer.drawIndexed(m_IndexCount, 1, 0, 0, 0);
	}

	void Model::loadModel(const std::string& filepath)
	{
		tinyobj::attrib_t attribute;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string error;

		if (!tinyobj::LoadObj(&attribute, &shapes, &materials, &error, filepath.c_str()))
		{
			throw std::runtime_error(error);
		}

		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		std::unordered_map<Vertex, uint32_t> uniqueVertices{};

		for (const auto& shape : shapes)
		{
			for (const auto& index : shape.mesh.indices)
			{
				Vertex vertex{};

				vertex.Position = {
					attribute.vertices[3 * index.vertex_index + 0],
					attribute.vertices[3 * index.vertex_index + 1],
					attribute.vertices[3 * index.vertex_index + 2]
				};

				vertex.TextureCoordinate = {
					attribute.texcoords[2 * index.texcoord_index + 0],
					1.0f - attribute.texcoords[2 * index.texcoord_index + 1]
				};

				vertex.Color = { 1.0f, 1.0f, 1.0f };

				auto [it, inserted] = uniqueVertices.try_emplace(vertex, static_cast<uint32_t>(vertices.size()));
				if (inserted)
				{
					vertices.push_back(vertex);
				}
				indices.push_back(it->second);
			}
		}

		m_IndexCount = static_cast<uint32_t>(indices.size());

		createVertexBuffers(vertices);
		createIndexBuffers(indices);
	}

	void Model::createVertexBuffers(const std::vector<Vertex>& vertices)
	{
		vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

		Buffer stagingBuffer{
			m_Context,
			bufferSize,
			vk::BufferUsageFlagBits::eTransferSrc,
			vk::MemoryPropertyFlagBits::eHostVisible |
			vk::MemoryPropertyFlagBits::eHostCoherent
		};

		void* data = stagingBuffer.Map();
		memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
		stagingBuffer.Unmap();

		m_VertexBuffer = std::make_unique<Buffer>(
			m_Context,
			bufferSize,
			vk::BufferUsageFlagBits::eVertexBuffer |
			vk::BufferUsageFlagBits::eTransferDst,
			vk::MemoryPropertyFlagBits::eDeviceLocal);

		m_VertexBuffer->CopyFrom(stagingBuffer, bufferSize);
	}

	void Model::createIndexBuffers(const std::vector<uint32_t>& indices)
	{
		vk::DeviceSize bufferSize = sizeof(indices[0]) * indices.size();

		Buffer stagingBuffer{
			m_Context,
			bufferSize,
			vk::BufferUsageFlagBits::eTransferSrc,
			vk::MemoryPropertyFlagBits::eHostVisible |
			vk::MemoryPropertyFlagBits::eHostCoherent
		};

		void* data = stagingBuffer.Map();
		memcpy(data, indices.data(), static_cast<size_t>(bufferSize));
		stagingBuffer.Unmap();

		m_IndexBuffer = std::make_unique<Buffer>(
			m_Context,
			bufferSize,
			vk::BufferUsageFlagBits::eIndexBuffer |
			vk::BufferUsageFlagBits::eTransferDst,
			vk::MemoryPropertyFlagBits::eDeviceLocal);

		m_IndexBuffer->CopyFrom(stagingBuffer, bufferSize);
	}

}
