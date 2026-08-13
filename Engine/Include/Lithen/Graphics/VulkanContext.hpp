#pragma once
#include "LithenPCH.hpp"

#include "Core/Window.hpp"

namespace Lithen {

	namespace {

		const std::vector<const char*> ValidationLayers = {
			"VK_LAYER_KHRONOS_validation"
		};

#ifdef NDEBUG
		constexpr bool EnableValidationLayers = false;
#else
		constexpr bool EnableValidationLayers = true;
#endif

	}

	class VKContext
	{
	public:
		VKContext(const Window& window);
		~VKContext();

		const vk::raii::Instance& GetInstance() const { return m_Instance; }
		const vk::raii::PhysicalDevice& GetPhysicalDevice() const { return m_PhysicalDevice; }
		const vk::raii::Device& GetDevice() const { return m_Device; }
		const vk::raii::SurfaceKHR& GetSurfaceKHR() const { return m_Surface; }
		const vk::raii::Queue& GetQueue() const { return m_Queue; }
		const uint32_t GetQueueIndex() const { return m_QueueIndex; }
		vk::SampleCountFlagBits GetMsaaSamples() const { return m_MsaaSamples; }

		void ExecuteImmediateCommand(const std::function<void(vk::raii::CommandBuffer&)>& function) const;
		uint32_t FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const;

	private:
		void createInstance();
		void setupDebugMessenger();
		void createSurface();
		void pickPhysicalDevice();
		void createLogicalDevice();
		void createCommandPool();

		std::vector<const char*> getRequiredInstanceExtensions();
		vk::SampleCountFlagBits getMaxUsableSampleCount() const;

		static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(
			vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			vk::DebugUtilsMessageTypeFlagsEXT messageTypes,
			const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData);

	private:
		const Window& m_Window;

		vk::raii::Context m_Context;
		vk::raii::Instance m_Instance = nullptr;
		vk::raii::DebugUtilsMessengerEXT m_DebugMessenger = nullptr;
		vk::raii::SurfaceKHR m_Surface = nullptr;

		vk::raii::PhysicalDevice m_PhysicalDevice = nullptr;

		vk::raii::Device m_Device = nullptr;
		vk::raii::Queue m_Queue = nullptr;
		uint32_t m_QueueIndex = ~0;

		vk::raii::CommandPool m_CommandPool = nullptr;

		vk::SampleCountFlagBits m_MsaaSamples = vk::SampleCountFlagBits::e1;

		std::vector<const char*> m_RequiredDeviceExtensions = {
			vk::KHRSwapchainExtensionName
		};
	};

}
