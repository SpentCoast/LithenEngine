#pragma once
#include "LithenPCH.hpp"

#include "Graphics/VulkanContext.hpp"

namespace Lithen {

	class Buffer
	{
	public:
		Buffer() = default;

		Buffer(const Buffer&) = delete;
		Buffer& operator=(const Buffer&) = delete;

		Buffer(Buffer&&) = default;
		Buffer& operator=(Buffer&&) = default;

		Buffer(const VKContext& vkContext, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);
		~Buffer() = default;

		void* Map();
		void Unmap();
		void CopyFrom(const Buffer& srcBuffer, vk::DeviceSize size);

		const vk::raii::Buffer& GetHandle() const { return m_Buffer; }
		vk::DeviceSize GetSize() const { return m_Size; }

	private:
		const VKContext* m_VKContext = nullptr;

		vk::DeviceSize m_Size = 0;
		vk::raii::Buffer m_Buffer = nullptr;
		vk::raii::DeviceMemory m_Memory = nullptr;
	};

}
