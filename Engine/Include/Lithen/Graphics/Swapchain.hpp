#pragma once
#include "LithenPCH.hpp"

#include "Core/Window.hpp"
#include "Graphics/VulkanContext.hpp"

namespace Lithen {

	class Swapchain
	{
	public:
		Swapchain(Window& window, const VKContext& vkContext);
		~Swapchain();

		void Recreate();

		const vk::raii::SwapchainKHR& GetHandle() const { return m_Swapchain; }
		const vk::Extent2D& GetExtent() const { return m_SwapChainExtent; }
		vk::Format GetFormat() const { return m_SwapChainSurfaceFormat.format; }
		const std::vector<vk::raii::ImageView>& GetImageViews() const { return m_SwapChainImageViews; }
		const std::vector<vk::Image>& GetImages() const { return m_SwapChainImages; }

	private:
		void create();
		void cleanup();

		static uint32_t chooseSwapMinImageCount(const vk::SurfaceCapabilitiesKHR& capabilities);
		vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);
		static vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
		static vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);

	private:
		Window& m_Window;
		const VKContext& m_VKContext;

		vk::raii::SwapchainKHR m_Swapchain = nullptr;
		std::vector<vk::Image> m_SwapChainImages;
		vk::SurfaceFormatKHR m_SwapChainSurfaceFormat;
		vk::Extent2D m_SwapChainExtent;
		std::vector<vk::raii::ImageView> m_SwapChainImageViews;
	};

}
