#pragma once
#include "LithenPCH.hpp"

#include "Graphics/VulkanContext.hpp"
#include "Graphics/Swapchain.hpp"
#include "Graphics/Buffer.hpp"
#include "Graphics/Texture.hpp"
#include "Graphics/GraphicsPipeline.hpp"
#include "Graphics/DescriptorManager.hpp"
#include "Graphics/Model.hpp"

namespace Lithen {

	namespace {

		constexpr int MAX_FRAMES_IN_FLIGHT = 2;

	}

	class Renderer
	{
	public:
		Renderer(Window& window, const VKContext& vkContext);
		~Renderer();

		void DrawFrame();

	private:
		void createGraphicsPipeline();
		void createCommandPool();
		void createDepthResources();
		void createCommandBuffers();
		void createSyncObjects();

		void recreateSwapchain();

		std::pair<vk::raii::Image, vk::raii::DeviceMemory> createImage(
			uint32_t width, uint32_t height,
			vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties);

		void recordCommandBuffer(uint32_t imageIndex);
		void transitionImageLayout(
			vk::Image image,
			vk::ImageLayout oldLayout,
			vk::ImageLayout newLayout,
			vk::AccessFlags2 srcAccessMask,
			vk::AccessFlags2 dstAccessMask,
			vk::PipelineStageFlags2 srcStageMask,
			vk::PipelineStageFlags2 dstStageMask,
			vk::ImageAspectFlags imageAspectFlags);

	private:
		Window& m_Window;
		const VKContext& m_Context;

		Swapchain m_Swapchain;

		std::unique_ptr<DescriptorManager> m_DescriptorManager;

		std::unique_ptr<GraphicsPipeline> m_GraphicsPipeline;

		vk::raii::CommandPool m_CommandPool = nullptr;
		std::vector<vk::raii::CommandBuffer> m_CommandBuffers;

		std::unique_ptr<Model> m_Model;

		Texture m_Texture;
		Texture m_ColorTexture;
		Texture m_DepthTexture;

		std::vector<vk::raii::Semaphore> m_PresentCompleteSemaphores;
		std::vector<vk::raii::Semaphore> m_RenderFinishedSemaphores;
		std::vector<vk::raii::Fence> m_InFlightFences;
		uint32_t m_FrameIndex = 0;
	};

}
