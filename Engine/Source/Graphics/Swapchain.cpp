#include "Graphics/Swapchain.hpp"

namespace Lithen {

	Swapchain::Swapchain(Window& window, const VKContext& vkContext)
		: m_Window{ window }, m_VKContext{ vkContext }
	{
		create();
	}

	Swapchain::~Swapchain()
	{
		cleanup();
	}

	void Swapchain::Recreate()
	{
		int width = 0, height = 0;
		glfwGetFramebufferSize(m_Window.GetWindowHandle(), &width, &height);
		while (width == 0 || height == 0)
		{
			glfwGetFramebufferSize(m_Window.GetWindowHandle(), &width, &height);
			glfwWaitEvents();
		}

		m_VKContext.GetDevice().waitIdle();

		cleanup();
		create();
	}

	void Swapchain::create()
	{
		auto surfaceCapabilities = m_VKContext.GetPhysicalDevice().getSurfaceCapabilitiesKHR(*m_VKContext.GetSurfaceKHR());
		m_SwapChainExtent = chooseSwapExtent(surfaceCapabilities);

		uint32_t minImageCount = chooseSwapMinImageCount(surfaceCapabilities);

		auto availableFormats = m_VKContext.GetPhysicalDevice().getSurfaceFormatsKHR(*m_VKContext.GetSurfaceKHR());
		m_SwapChainSurfaceFormat = chooseSwapSurfaceFormat(availableFormats);

		auto availablePresentModes = m_VKContext.GetPhysicalDevice().getSurfacePresentModesKHR(*m_VKContext.GetSurfaceKHR());
		auto swapChainPresentMode = chooseSwapPresentMode(availablePresentModes);

		vk::SwapchainCreateInfoKHR swapChainCreateInfo{
			{},
			*m_VKContext.GetSurfaceKHR(),
			minImageCount,
			m_SwapChainSurfaceFormat.format,
			m_SwapChainSurfaceFormat.colorSpace,
			m_SwapChainExtent,
			1,
			vk::ImageUsageFlagBits::eColorAttachment,
			vk::SharingMode::eExclusive,
			{},
			surfaceCapabilities.currentTransform,
			vk::CompositeAlphaFlagBitsKHR::eOpaque,
			swapChainPresentMode,
			true
		};

		m_Swapchain = vk::raii::SwapchainKHR{ m_VKContext.GetDevice(), swapChainCreateInfo };
		m_SwapChainImages = m_Swapchain.getImages();

		assert(m_SwapChainImageViews.empty());

		m_SwapChainImageViews.reserve(m_SwapChainImages.size());
		for (auto& image : m_SwapChainImages)
		{
			vk::ImageViewCreateInfo viewInfo{
				{},
				image,
				vk::ImageViewType::e2D,
				m_SwapChainSurfaceFormat.format,
				{},
				vk::ImageSubresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 }
			};

			m_SwapChainImageViews.emplace_back(m_VKContext.GetDevice(), viewInfo);
		}
	}

	void Swapchain::cleanup()
	{
		m_SwapChainImageViews.clear();
		m_Swapchain = nullptr;
	}

	uint32_t Swapchain::chooseSwapMinImageCount(const vk::SurfaceCapabilitiesKHR& capabilities)
	{
		auto minImageCount = std::max(3U, capabilities.minImageCount);
		if ((0 < capabilities.maxImageCount) && (capabilities.maxImageCount < minImageCount))
		{
			minImageCount = capabilities.maxImageCount;
		}
		return minImageCount;
	}

	vk::Extent2D Swapchain::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities)
	{
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			return capabilities.currentExtent;
		}

		int width, height;
		glfwGetFramebufferSize(m_Window.GetWindowHandle(), &width, &height);

		return {
			std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
			std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
		};
	}

	vk::SurfaceFormatKHR Swapchain::chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats)
	{
		if (auto it = std::ranges::find_if(availableFormats,
			[](const auto& format)
			{
				return format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
			}); it != availableFormats.end())
		{
			return *it;
		}

		return availableFormats[0];
	}

	vk::PresentModeKHR Swapchain::chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes)
	{
		if (std::ranges::any_of(availablePresentModes,
			[](const auto& presentMode)
			{
				return presentMode == vk::PresentModeKHR::eMailbox;
			}))
		{
			return vk::PresentModeKHR::eMailbox;
		}

		return vk::PresentModeKHR::eFifo;
	}

}
