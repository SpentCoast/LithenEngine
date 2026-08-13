#include "Graphics/Model.hpp"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tiny_gltf.h>

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
		tinygltf::Model model;
		tinygltf::TinyGLTF loader;
		std::string error, warn;

		bool ret = loader.LoadBinaryFromFile(&model, &error, &warn, filepath);

		if (!warn.empty())
		{
			std::cout << "glTF warning: " << warn << std::endl;
		}
		if (!error.empty())
		{
			std::cerr << "glTF error: " << error << std::endl;
		}
		if (!ret)
		{
			throw std::runtime_error("Failed to load glTF model");
		}

		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		std::unordered_map<Vertex, uint32_t> uniqueVertices{};

		for (const auto& mesh : model.meshes)
		{
			for (const auto& primitive : mesh.primitives)
			{
				const tinygltf::Accessor& indexAccessor = model.accessors[primitive.indices];
				const tinygltf::BufferView& indexBufferView = model.bufferViews[indexAccessor.bufferView];
				const tinygltf::Buffer& indexBuffer = model.buffers[indexBufferView.buffer];

				const tinygltf::Accessor& positionAccessor = model.accessors[primitive.attributes.at("POSITION")];
				const tinygltf::BufferView& positionBufferView = model.bufferViews[positionAccessor.bufferView];
				const tinygltf::Buffer& positionBuffer = model.buffers[positionBufferView.buffer];

				bool hasTexCoords = primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end();
				const tinygltf::Accessor* texCoordAccessor = nullptr;
				const tinygltf::BufferView* texCoordBufferView = nullptr;
				const tinygltf::Buffer* texCoordBuffer = nullptr;

				if (hasTexCoords)
				{
					texCoordAccessor = &model.accessors[primitive.attributes.at("TEXCOORD_0")];
					texCoordBufferView = &model.bufferViews[texCoordAccessor->bufferView];
					texCoordBuffer = &model.buffers[texCoordBufferView->buffer];
				}

				uint32_t baseVertex = static_cast<uint32_t>(vertices.size());

				for (size_t i = 0; i < positionAccessor.count; ++i)
				{
					Vertex vertex{};

					const float* position = reinterpret_cast<const float*>(&positionBuffer.data[positionBufferView.byteOffset + positionAccessor.byteOffset + i * 12]);
					vertex.Position = { position[0], position[1], position[2] };

					if (hasTexCoords)
					{
						const float* texCoord = reinterpret_cast<const float*>(&texCoordBuffer->data[texCoordBufferView->byteOffset + texCoordAccessor->byteOffset + i * 8]);
						vertex.TextureCoordinate = { texCoord[0], texCoord[1] };
					}
					else
					{
						vertex.TextureCoordinate = { 0.0f, 0.0f };
					}

					vertex.Color = { 1.0f, 1.0f, 1.0f };

					vertices.push_back(vertex);
				}

				const unsigned char* indexData = &indexBuffer.data[indexBufferView.byteOffset + indexAccessor.byteOffset];
				size_t indexCount = indexAccessor.count;
				size_t indexStride = 0;

				if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
				{
					indexStride = sizeof(uint16_t);
				}
				else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
				{
					indexStride = sizeof(uint32_t);
				}
				else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
				{
					indexStride = sizeof(uint8_t);
				}
				else
				{
					throw std::runtime_error("Unsupported index component type");
				}

				indices.reserve(indices.size() + indexCount);

				for (size_t i = 0; i < indexCount; ++i)
				{
					uint32_t index = 0;

					if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
					{
						index = *reinterpret_cast<const uint16_t*>(indexData + i * indexStride);
					}
					else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
					{
						index = *reinterpret_cast<const uint32_t*>(indexData + i * indexStride);
					}
					else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
					{
						index = *reinterpret_cast<const uint8_t*>(indexData + i * indexStride);
					}

					indices.push_back(baseVertex + index);
				}
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
