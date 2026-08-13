#pragma once
#include "LithenPCH.hpp"

#include "Graphics/VulkanContext.hpp"
#include "Graphics/Buffer.hpp"

namespace Lithen {

	class Texture
	{
	public:
		Texture() = default;

		Texture(const Texture&) = delete;
		Texture& operator=(const Texture&) = delete;

		Texture(Texture&&) = default;
		Texture& operator=(Texture&&) = default;

		static Texture LoadFromFile(const VKContext& context, const std::string& filepath);

		static Texture CreateAttachment(
			const VKContext& context, 
			uint32_t width, uint32_t height,
			vk::SampleCountFlagBits numSamples,
			vk::Format format,
			vk::ImageUsageFlags usage,
			vk::ImageAspectFlags aspectFlags);

		const vk::raii::ImageView& GetImageView() const { return m_ImageView; }
		const vk::raii::Sampler& GetSampler() const { return m_Sampler; }
		const vk::raii::Image& GetImage() const { return m_Image; }

		static vk::Format FindDepthFormat(const VKContext& context);

	private:
		Texture(
			const VKContext& context,
			uint32_t width,
			uint32_t height,
			uint32_t mipLevels,
			vk::SampleCountFlagBits numSamples,
			vk::Format format,
			vk::ImageTiling tiling,
			vk::ImageUsageFlags usage,
			vk::MemoryPropertyFlags properties,
			vk::ImageAspectFlags aspectFlags,
			bool createSampler);

		static void transitionImageLayout(const VKContext& context, vk::Image image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t mipLevels);
		static void copyBufferToImage(const VKContext& context, vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height);
		static vk::Format findSupportedFormat(const VKContext& context, const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);
		static void generateMipmaps(const VKContext& context, vk::Image image, vk::Format imageFormat, int32_t textureWidth, int32_t textureHeight, uint32_t mipLevels);

	private:
		vk::raii::Image m_Image = nullptr;
		vk::raii::DeviceMemory m_Memory = nullptr;
		vk::raii::ImageView m_ImageView = nullptr;
		vk::raii::Sampler m_Sampler = nullptr;
	};

}
