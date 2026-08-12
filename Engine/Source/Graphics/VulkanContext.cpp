#include "Graphics/VulkanContext.hpp"

namespace Lithen {

	VKContext::VKContext(const Window& window)
		: m_Window{ window }
	{
		createInstance();
		setupDebugMessenger();
		createSurface();
		pickPhysicalDevice();
		createLogicalDevice();
		createCommandPool();
	}

	VKContext::~VKContext()
	{
	}

	void VKContext::ExecuteImmediateCommand(const std::function<void(vk::raii::CommandBuffer&)>& function) const
	{
		vk::CommandBufferAllocateInfo allocateInfo{
			m_CommandPool,
			vk::CommandBufferLevel::ePrimary,
			1
		};
		vk::raii::CommandBuffer commandCopyBuffer = std::move(m_Device.allocateCommandBuffers(allocateInfo).front());

		commandCopyBuffer.begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
		function(commandCopyBuffer);
		commandCopyBuffer.end();

		m_Queue.submit(vk::SubmitInfo{ {}, {}, *commandCopyBuffer }, nullptr);
		m_Queue.waitIdle();
	}

	uint32_t VKContext::FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const
	{
		vk::PhysicalDeviceMemoryProperties memoryProperties = m_PhysicalDevice.getMemoryProperties();

		for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
		{
			if ((typeFilter & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				return i;
			}
		}

		throw std::runtime_error("Failed to find suitable memory type!");
	}

	void VKContext::createInstance()
	{
		constexpr vk::ApplicationInfo appInfo{
			"Lithen Application",
			VK_MAKE_VERSION(1, 0, 0),
			"Lithen Engine",
			VK_MAKE_VERSION(1, 0, 0),
			vk::ApiVersion14
		};

		std::vector<const char*> requiredLayers;
		if (EnableValidationLayers)
		{
			requiredLayers.assign(ValidationLayers.begin(), ValidationLayers.end());
		}

		auto layerProperties = m_Context.enumerateInstanceLayerProperties();
		if (auto it = std::ranges::find_if(requiredLayers,
			[&layerProperties](const auto& requiredLayer)
			{
				return std::ranges::none_of(layerProperties,
					[requiredLayer](const auto& layerProperty)
					{
						return strcmp(layerProperty.layerName, requiredLayer) == 0;
					});
			}); it != requiredLayers.end())
		{
			throw std::runtime_error("Required layer not supported: " + std::string{ *it });
		}

		auto requiredExtensions = getRequiredInstanceExtensions();

		auto extensionProperties = m_Context.enumerateInstanceExtensionProperties();
		if (auto it = std::ranges::find_if(requiredExtensions,
			[&extensionProperties](const auto& requiredExtension)
			{
				return std::ranges::none_of(extensionProperties,
					[requiredExtension](const auto& extensionProperty)
					{
						return strcmp(extensionProperty.extensionName, requiredExtension) == 0;
					});
			}); it != requiredExtensions.end())
		{
			throw std::runtime_error("Required extension not supported: " + std::string{ *it });
		}

		vk::InstanceCreateInfo createInfo{
			{},
			&appInfo,
			requiredLayers,
			requiredExtensions
		};

		m_Instance = vk::raii::Instance{ m_Context, createInfo };
	}

	void VKContext::setupDebugMessenger()
	{
		if (!EnableValidationLayers)
		{
			return;
		}

		vk::DebugUtilsMessageSeverityFlagsEXT messageSeverity{
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
		};
		vk::DebugUtilsMessageTypeFlagsEXT messageType{
			vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
			vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
			vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance
		};

		vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{
			{},
			messageSeverity,
			messageType,
			&debugCallback
		};

		m_DebugMessenger = vk::raii::DebugUtilsMessengerEXT{ m_Instance, {} };
	}

	void VKContext::createSurface()
	{
		VkSurfaceKHR _surface;
		if (glfwCreateWindowSurface(*m_Instance, m_Window.GetWindowHandle(), nullptr, &_surface) != 0)
		{
			throw std::runtime_error("Failed to create window surface!");
		}
		m_Surface = vk::raii::SurfaceKHR{ m_Instance, _surface };
	}

	void VKContext::pickPhysicalDevice()
	{
		auto physicalDevices = m_Instance.enumeratePhysicalDevices();
		if (physicalDevices.empty())
		{
			throw std::runtime_error("Failed to find a GPU with Vulkan support!");
		}

		std::multimap<int, vk::raii::PhysicalDevice> candidates;
		for (const auto& physicalDevice : physicalDevices)
		{
			auto deviceProperties = physicalDevice.getProperties();
			auto deviceFeatures = physicalDevice.getFeatures();
			uint32_t score = 0;

			if (deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
			{
				score += 1000;
			}

			score += deviceProperties.limits.maxImageDimension2D;

			if (!deviceFeatures.geometryShader || !deviceFeatures.samplerAnisotropy)
			{
				continue;
			}
			candidates.insert(std::make_pair(score, physicalDevice));
		}

		if (!candidates.empty() && candidates.rbegin()->first > 0)
		{
			m_PhysicalDevice = candidates.rbegin()->second;
			m_MsaaSamples = getMaxUsableSampleCount();
		}
		else
		{
			throw std::runtime_error("Failed to find a suitable GPU!");
		}
	}

	void VKContext::createLogicalDevice()
	{
		auto queueFamilyProperties = m_PhysicalDevice.getQueueFamilyProperties();
		for (uint32_t qfpIndex = 0; qfpIndex < queueFamilyProperties.size(); ++qfpIndex)
		{
			if ((queueFamilyProperties[qfpIndex].queueFlags & vk::QueueFlagBits::eGraphics) &&
				m_PhysicalDevice.getSurfaceSupportKHR(qfpIndex, *m_Surface))
			{
				m_QueueIndex = qfpIndex;
				break;
			}
		}
		if (m_QueueIndex == ~0)
		{
			throw std::runtime_error("Could not find queue for graphics and present -> terminating");
		}

		vk::StructureChain<
			vk::PhysicalDeviceFeatures2,
			vk::PhysicalDeviceVulkan11Features,
			vk::PhysicalDeviceVulkan13Features,
			vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> featureChain;
		featureChain.get<vk::PhysicalDeviceFeatures2>().features.samplerAnisotropy = true;
		featureChain.get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters = true;
		featureChain.get<vk::PhysicalDeviceVulkan13Features>().synchronization2 = true;
		featureChain.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering = true;
		featureChain.get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState = true;

		float queuePriority = 0.5f;
		vk::DeviceQueueCreateInfo deviceQueueCreateInfo{
			{},
			m_QueueIndex,
			1,
			&queuePriority
		};
		vk::DeviceCreateInfo deviceCreateInfo{
			{},
			deviceQueueCreateInfo,
			{},
			m_RequiredDeviceExtensions,
			{},
			&featureChain.get<vk::PhysicalDeviceFeatures2>()
		};

		m_Device = vk::raii::Device{ m_PhysicalDevice, deviceCreateInfo };
		m_Queue = vk::raii::Queue{ m_Device, m_QueueIndex, 0 };
	}

	void VKContext::createCommandPool()
	{
		vk::CommandPoolCreateInfo poolInfo{ vk::CommandPoolCreateFlagBits::eResetCommandBuffer, m_QueueIndex };
		m_CommandPool = vk::raii::CommandPool{ m_Device, poolInfo };
	}

	std::vector<const char*> VKContext::getRequiredInstanceExtensions()
	{
		uint32_t glfwExtensionCount = 0;
		auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
		if (EnableValidationLayers)
		{
			extensions.push_back(vk::EXTDebugUtilsExtensionName);
		}

		return extensions;
	}

	vk::SampleCountFlagBits VKContext::getMaxUsableSampleCount() const
	{
		vk::PhysicalDeviceProperties physicalDeviceProperties = m_PhysicalDevice.getProperties();

		vk::SampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
		if (counts & vk::SampleCountFlagBits::e64) { return vk::SampleCountFlagBits::e64; }
		if (counts & vk::SampleCountFlagBits::e32) { return vk::SampleCountFlagBits::e32; }
		if (counts & vk::SampleCountFlagBits::e16) { return vk::SampleCountFlagBits::e16; }
		if (counts & vk::SampleCountFlagBits::e8) { return vk::SampleCountFlagBits::e8; }
		if (counts & vk::SampleCountFlagBits::e4) { return vk::SampleCountFlagBits::e4; }
		if (counts & vk::SampleCountFlagBits::e2) { return vk::SampleCountFlagBits::e2; }

		return vk::SampleCountFlagBits::e1;
	}

	VKAPI_ATTR vk::Bool32 VKAPI_CALL VKContext::debugCallback(
		vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		vk::DebugUtilsMessageTypeFlagsEXT messageTypes,
		const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
	{
		std::cerr << "Validation layer: Type " << to_string(messageTypes) << " msg: " << pCallbackData->pMessage << std::endl;

		return vk::False;
	}

}
