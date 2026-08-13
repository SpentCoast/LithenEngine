#pragma once
#include "LithenPCH.hpp"

#include "Core/GameObject.hpp"

#include "Graphics/VulkanContext.hpp"
#include "Graphics/Buffer.hpp"
#include "Graphics/Texture.hpp"
#include "Graphics/Camera.hpp"

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
		DescriptorManager(const VKContext& context, uint32_t maxFramesInFlight, size_t maxObjects);
		~DescriptorManager() = default;

		void UpdateUBO(uint32_t currentFrame, size_t objectIndex, const glm::mat4& modelMatrix, const Camera& camera);
		
		void CreateObjectDescriptorSets(const std::vector<GameObject>& gameObjects, const std::vector<std::shared_ptr<Texture>>& textures);

		vk::DescriptorSetLayout GetLayout() const { return vk::DescriptorSetLayout{ *m_DescriptorSetLayout }; }
		vk::DescriptorSet GetSet(size_t objectIndex, uint32_t currentFrame) const { return *m_ObjectDescriptorSets[objectIndex][currentFrame]; }

	private:
		void createLayout();
		void createPool(size_t maxObjects);

	private:
		const VKContext& m_Context;
		
		uint32_t m_MaxFramesInFlight;
		size_t m_MaxObjects;

		vk::raii::DescriptorSetLayout m_DescriptorSetLayout = nullptr;
		vk::raii::DescriptorPool m_DescriptorPool = nullptr;

		struct ObjectResources
		{
			std::vector<Buffer> UniformBuffers;
			std::vector<void*> UniformBuffersMapped;
		};
		std::vector<ObjectResources> m_ObjectResources;
		std::vector<std::vector<vk::raii::DescriptorSet>> m_ObjectDescriptorSets;
	};

}
