#include "Graphics/GraphicsPipeline.hpp"

namespace Lithen {

	GraphicsPipeline::GraphicsPipeline(
		const VKContext& context,
		const std::string& shaderPath,
		vk::SampleCountFlagBits msaaSamples,
		vk::DescriptorSetLayout descriptorLayout,
		vk::Format swapchainFormat,
		vk::Format depthFormat,
		const vk::PipelineVertexInputStateCreateInfo& vertexInputInfo)
		: m_Context{ context }
	{
		auto shaderModule = createShaderModule(readFile(shaderPath));

		vk::PipelineShaderStageCreateInfo vertShaderStageInfo{ {}, vk::ShaderStageFlagBits::eVertex, *shaderModule, "vertMain" };
		vk::PipelineShaderStageCreateInfo fragShaderStageInfo{ {}, vk::ShaderStageFlagBits::eFragment, *shaderModule, "fragMain" };
		vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

		vk::PipelineInputAssemblyStateCreateInfo inputAssembly{ {}, vk::PrimitiveTopology::eTriangleList };
		vk::PipelineViewportStateCreateInfo viewportState{ {}, 1, {}, 1 };

		vk::PipelineRasterizationStateCreateInfo rasterizer{
			{},
			vk::False,
			vk::False,
			vk::PolygonMode::eFill,
			vk::CullModeFlagBits::eNone,
			vk::FrontFace::eCounterClockwise,
			vk::False,
			{}, {}, {},
			1.0f
		};

		vk::PipelineMultisampleStateCreateInfo multisampling{ {}, msaaSamples, vk::False };

		vk::PipelineDepthStencilStateCreateInfo depthStencil{
			{},
			vk::True,
			vk::True,
			vk::CompareOp::eLess,
			vk::False,
			vk::False
		};

		vk::PipelineColorBlendAttachmentState colorBlendAttachment{
			vk::True,
			vk::BlendFactor::eSrcAlpha,
			vk::BlendFactor::eOneMinusSrcAlpha,
			vk::BlendOp::eAdd,
			vk::BlendFactor::eOne,
			vk::BlendFactor::eZero,
			vk::BlendOp::eAdd,
			vk::ColorComponentFlags{
				vk::ColorComponentFlagBits::eR |
				vk::ColorComponentFlagBits::eG |
				vk::ColorComponentFlagBits::eB |
				vk::ColorComponentFlagBits::eA
			}
		};

		vk::PipelineColorBlendStateCreateInfo colorBlending{
			{},
			vk::False,
			vk::LogicOp::eCopy,
			1,
			&colorBlendAttachment
		};

		std::vector<vk::DynamicState> dynamicStates = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
		vk::PipelineDynamicStateCreateInfo dynamicState{ {}, dynamicStates };

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{ {}, 1, &descriptorLayout, 0 };
		m_PipelineLayout = vk::raii::PipelineLayout{ m_Context.GetDevice(), pipelineLayoutInfo };

		vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> pipelineCreateInfoChain = {
			{
				{},
				2,
				shaderStages,
				&vertexInputInfo,
				&inputAssembly,
				{},
				&viewportState,
				&rasterizer,
				&multisampling,
				&depthStencil,
				&colorBlending,
				&dynamicState,
				*m_PipelineLayout,
				{}, {}, {}, {}
			},
			{
				{},
				1,
				&swapchainFormat,
				depthFormat
			}
		};

		m_Pipeline = vk::raii::Pipeline{ m_Context.GetDevice(), nullptr, pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>() };
	}

	std::vector<char> GraphicsPipeline::readFile(const std::string& filename)
	{
		std::ifstream file(filename, std::ios::ate | std::ios::binary);
		if (!file.is_open())
		{
			throw std::runtime_error("Failed to open file!");
		}

		std::vector<char> buffer(file.tellg());
		file.seekg(0, std::ios::beg);
		file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
		file.close();

		return buffer;
	}

	vk::raii::ShaderModule GraphicsPipeline::createShaderModule(const std::vector<char>& code) const
	{
		vk::ShaderModuleCreateInfo createInfo{
			{},
			code.size() * sizeof(char),
			reinterpret_cast<const uint32_t*>(code.data())
		};
		vk::raii::ShaderModule shaderModule{ m_Context.GetDevice(), createInfo };

		return shaderModule;
	}


}
