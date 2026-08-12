#include "Graphics/Buffer.hpp"

namespace Lithen {

	Buffer::Buffer(const VKContext& vkContext, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties)
		: m_VKContext{ &vkContext }, m_Size{ size }
	{
		vk::BufferCreateInfo bufferInfo{ {}, size, usage, vk::SharingMode::eExclusive };
		m_Buffer = vk::raii::Buffer(m_VKContext->GetDevice(), bufferInfo);

		vk::MemoryRequirements memoryRequirements = m_Buffer.getMemoryRequirements();
		vk::MemoryAllocateInfo allocateInfo{ memoryRequirements.size, m_VKContext->FindMemoryType(memoryRequirements.memoryTypeBits, properties) };

		m_Memory = vk::raii::DeviceMemory(m_VKContext->GetDevice(), allocateInfo);
		m_Buffer.bindMemory(*m_Memory, 0);
	}

	void* Buffer::Map()
	{
		return m_Memory.mapMemory(0, m_Size);
	}

	void Buffer::Unmap()
	{
		m_Memory.unmapMemory();
	}

	void Buffer::CopyFrom(const Buffer& srcBuffer, vk::DeviceSize size)
	{
		m_VKContext->ExecuteImmediateCommand([&](vk::raii::CommandBuffer& command)
			{
				vk::BufferCopy copyRegion{ 0, 0, size };
				command.copyBuffer(*srcBuffer.GetHandle(), *m_Buffer, copyRegion);
			});
	}

}
