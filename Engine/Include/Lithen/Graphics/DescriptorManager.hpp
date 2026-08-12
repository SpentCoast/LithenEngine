#pragma once
#include "LithenPCH.hpp"

#include "Graphics/VulkanContext.hpp"
#include "Graphics/Buffer.hpp"
#include "Graphics/Texture.hpp"

namespace Lithen {

	namespace {

		struct UniformBufferObject
		{
			glm::mat4 Model;
			glm::mat4 View;
			glm::mat4 Projection;
		};

	}

	class DescriptorManager
	{
	public:
		DescriptorManager(const VKContext& context, const Texture& texture, uint32_t maxFramesInFlight);
		~DescriptorManager() = default;

		void UpdateUBO(uint32_t currentFrame, const vk::Extent2D& extent);

		const vk::raii::DescriptorSetLayout& GetLayout() const { return m_DescriptorSetLayout; }
		const vk::raii::DescriptorSet& GetSet(uint32_t frameIndex) const { return m_DescriptorSets[frameIndex]; }

	private:
		void createLayout();
		void createUniformBuffers();
		void createPool();
		void createSets(const Texture& texture);

	private:
		const VKContext& m_Context;
		
		uint32_t m_MaxFramesInFlight;

		vk::raii::DescriptorSetLayout m_DescriptorSetLayout = nullptr;
		vk::raii::DescriptorPool m_DescriptorPool = nullptr;
		std::vector<vk::raii::DescriptorSet> m_DescriptorSets;

		std::vector<Buffer> m_UniformBuffers;
		std::vector<void*> m_UniformBuffersMapped;
	};

}
