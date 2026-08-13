#include "Graphics/DescriptorManager.hpp"

namespace Lithen {

	DescriptorManager::DescriptorManager(const VKContext& context, uint32_t maxFramesInFlight, size_t maxObjects)
		: m_Context{ context }, m_MaxFramesInFlight{ maxFramesInFlight }
	{
		createLayout();
		createPool(maxObjects);

		m_ObjectResources.resize(maxObjects);
		for (size_t objIdx = 0; objIdx < maxObjects; ++objIdx)
		{
			m_ObjectResources[objIdx].UniformBuffers.reserve(maxFramesInFlight);
			m_ObjectResources[objIdx].UniformBuffersMapped.reserve(maxFramesInFlight);

			for (size_t i = 0; i < maxFramesInFlight; ++i)
			{
				vk::DeviceSize bufferSize = sizeof(UniformBufferObject);
				Buffer buffer{
					m_Context,
					bufferSize,
					vk::BufferUsageFlagBits::eUniformBuffer,
					vk::MemoryPropertyFlagBits::eHostVisible |
					vk::MemoryPropertyFlagBits::eHostCoherent
				};
				m_ObjectResources[objIdx].UniformBuffers.emplace_back(std::move(buffer));
				m_ObjectResources[objIdx].UniformBuffersMapped.push_back(m_ObjectResources[objIdx].UniformBuffers.back().Map());
			}
		}
	}

	void DescriptorManager::UpdateUBO(uint32_t currentFrame, size_t objectIndex, const glm::mat4& modelMatrix, const vk::Extent2D& extent)
	{
		static auto startTime = std::chrono::high_resolution_clock::now();
		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

		UniformBufferObject ubo{};
		ubo.Model = modelMatrix;
		ubo.View = glm::lookAt(glm::vec3(4.0f, 4.0f, 2.0f), glm::vec3(0.0f, 0.0f, -0.5f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.Projection = glm::perspective(
			glm::radians(45.0f),
			static_cast<float>(extent.width) / static_cast<float>(extent.height),
			0.1f, 10.0f);
		ubo.Projection[1][1] *= -1;

		memcpy(m_ObjectResources[objectIndex].UniformBuffersMapped[currentFrame], &ubo, sizeof(ubo));
	}

	void DescriptorManager::CreateObjectDescriptorSets(const std::vector<GameObject>& gameObjects, const std::vector<std::shared_ptr<Texture>>& textures)
	{
		size_t numObjects = gameObjects.size();
		m_ObjectDescriptorSets.resize(numObjects);

		for (size_t objIdx = 0; objIdx < numObjects; ++objIdx)
		{
			std::vector<vk::DescriptorSetLayout> layouts(m_MaxFramesInFlight, *m_DescriptorSetLayout);
			vk::DescriptorSetAllocateInfo allocateInfo{ *m_DescriptorPool, layouts };

			m_ObjectDescriptorSets[objIdx] = m_Context.GetDevice().allocateDescriptorSets(allocateInfo);

			for (size_t i = 0; i < m_MaxFramesInFlight; ++i)
			{
				vk::DescriptorBufferInfo bufferInfo{
					m_ObjectResources[objIdx].UniformBuffers[i].GetHandle(),
					0,
					sizeof(UniformBufferObject)
				};

				const auto& texture = textures[objIdx];
				vk::DescriptorImageInfo imageInfo{
					*texture->GetSampler(),
					*texture->GetImageView(),
					vk::ImageLayout::eShaderReadOnlyOptimal
				};

				std::array<vk::WriteDescriptorSet, 2> descriptorWrites{
					vk::WriteDescriptorSet{
						*m_ObjectDescriptorSets[objIdx][i],
						0,
						0,
						1,
						vk::DescriptorType::eUniformBuffer,
						{},
						&bufferInfo
					},
					vk::WriteDescriptorSet{
						*m_ObjectDescriptorSets[objIdx][i],
						1,
						0,
						1,
						vk::DescriptorType::eCombinedImageSampler,
						&imageInfo,
					}
				};

				m_Context.GetDevice().updateDescriptorSets(descriptorWrites, {});
			}
		}
	}

	void DescriptorManager::createLayout()
	{
		std::array<vk::DescriptorSetLayoutBinding, 2> bindings{
			vk::DescriptorSetLayoutBinding{
				0,
				vk::DescriptorType::eUniformBuffer,
				1,
				vk::ShaderStageFlagBits::eVertex
			},
			vk::DescriptorSetLayoutBinding{
				1,
				vk::DescriptorType::eCombinedImageSampler,
				1,
				vk::ShaderStageFlagBits::eFragment
			}
		};

		vk::DescriptorSetLayoutCreateInfo layoutInfo{ {}, bindings };
		m_DescriptorSetLayout = vk::raii::DescriptorSetLayout{ m_Context.GetDevice(), layoutInfo };
	}

	void DescriptorManager::createPool(size_t maxObjects)
	{
		uint32_t totalSets = static_cast<uint32_t>(maxObjects * m_MaxFramesInFlight);
		std::array<vk::DescriptorPoolSize, 2> poolSize{
			vk::DescriptorPoolSize{ vk::DescriptorType::eUniformBuffer, totalSets },
			vk::DescriptorPoolSize{ vk::DescriptorType::eCombinedImageSampler, totalSets },
		};
		vk::DescriptorPoolCreateInfo poolInfo{
			vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
			totalSets,
			poolSize
		};
		m_DescriptorPool = vk::raii::DescriptorPool{ m_Context.GetDevice(), poolInfo };
	}

}
