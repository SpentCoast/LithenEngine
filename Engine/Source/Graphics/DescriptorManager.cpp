#include "Graphics/DescriptorManager.hpp"

namespace Lithen {

	DescriptorManager::DescriptorManager(const VKContext& context, const Texture& texture, uint32_t maxFramesInFlight)
		: m_Context{ context }, m_MaxFramesInFlight{ maxFramesInFlight }
	{
		createLayout();
		createUniformBuffers();
		createPool();
		createSets(texture);
	}

	void DescriptorManager::UpdateUBO(uint32_t currentFrame, const vk::Extent2D& extent)
	{
		static auto startTime = std::chrono::high_resolution_clock::now();
		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

		UniformBufferObject ubo{};
		ubo.Model = glm::rotate(glm::mat4(1.0f), time * glm::radians(10.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.View = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.Projection = glm::perspective(
			glm::radians(45.0f),
			static_cast<float>(extent.width) / static_cast<float>(extent.height),
			0.1f, 10.0f);
		ubo.Projection[1][1] *= -1;

		memcpy(m_UniformBuffersMapped[currentFrame], &ubo, sizeof(ubo));
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

	void DescriptorManager::createUniformBuffers()
	{
		for (size_t i = 0; i < m_MaxFramesInFlight; ++i)
		{
			vk::DeviceSize bufferSize = sizeof(UniformBufferObject);
			Buffer buffer{
				m_Context,
				bufferSize,
				vk::BufferUsageFlagBits::eUniformBuffer,
				vk::MemoryPropertyFlagBits::eHostVisible |
				vk::MemoryPropertyFlagBits::eHostCoherent
			};
			m_UniformBuffers.emplace_back(std::move(buffer));
			m_UniformBuffersMapped.push_back(m_UniformBuffers.back().Map());
		}
	}

	void DescriptorManager::createPool()
	{
		std::array<vk::DescriptorPoolSize, 2> poolSize{
			vk::DescriptorPoolSize{ vk::DescriptorType::eUniformBuffer, m_MaxFramesInFlight },
			vk::DescriptorPoolSize{ vk::DescriptorType::eCombinedImageSampler, m_MaxFramesInFlight },
		};
		vk::DescriptorPoolCreateInfo poolInfo{
			vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
			m_MaxFramesInFlight,
			poolSize
		};
		m_DescriptorPool = vk::raii::DescriptorPool{ m_Context.GetDevice(), poolInfo };
	}

	void DescriptorManager::createSets(const Texture& texture)
	{
		std::vector<vk::DescriptorSetLayout> layouts(m_MaxFramesInFlight, *m_DescriptorSetLayout);
		vk::DescriptorSetAllocateInfo allocateInfo{ *m_DescriptorPool, layouts };

		m_DescriptorSets = m_Context.GetDevice().allocateDescriptorSets(allocateInfo);

		for (size_t i = 0; i < m_MaxFramesInFlight; ++i)
		{
			vk::DescriptorBufferInfo bufferInfo{ m_UniformBuffers[i].GetHandle(), 0, sizeof(UniformBufferObject) };
			vk::DescriptorImageInfo imageInfo{ *texture.GetSampler(), *texture.GetImageView(), vk::ImageLayout::eShaderReadOnlyOptimal };

			std::array<vk::WriteDescriptorSet, 2> descriptorWrites{
				vk::WriteDescriptorSet{
					*m_DescriptorSets[i],
					0,
					0,
					1,
					vk::DescriptorType::eUniformBuffer,
					{},
					&bufferInfo
				},
				vk::WriteDescriptorSet{
					*m_DescriptorSets[i],
					1,
					0,
					1,
					vk::DescriptorType::eCombinedImageSampler,
					&imageInfo
				}
			};

			m_Context.GetDevice().updateDescriptorSets(descriptorWrites, {});
		}
	}

}
