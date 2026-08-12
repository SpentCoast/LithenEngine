#pragma once
#include "LithenPCH.hpp"

#include "Graphics/VulkanContext.hpp"

namespace Lithen {

	class GraphicsPipeline
	{
	public:
		GraphicsPipeline(
			const VKContext& context,
			const std::string& shaderPath,
			vk::SampleCountFlagBits numSamples,
			vk::DescriptorSetLayout descriptorLayout,
			vk::Format swapchainFormat,
			vk::Format depthFormat,
			const vk::PipelineVertexInputStateCreateInfo& vertexInputInfo);
		~GraphicsPipeline() = default;

		const vk::raii::Pipeline& GetHandle() const { return m_Pipeline; }
		const vk::raii::PipelineLayout& GetLayout() const { return m_PipelineLayout; }

	private:
		static std::vector<char> readFile(const std::string& filename);
		vk::raii::ShaderModule createShaderModule(const std::vector<char>& code) const;

	private:
		const VKContext& m_Context;
		
		vk::raii::PipelineLayout m_PipelineLayout = nullptr;
		vk::raii::Pipeline m_Pipeline = nullptr;
	};

}
