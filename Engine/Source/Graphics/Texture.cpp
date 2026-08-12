#include "Graphics/Texture.hpp"

namespace Lithen {

	Texture::Texture(
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
		bool createSampler)
	{
		vk::ImageCreateInfo imageInfo{
			{},
			vk::ImageType::e2D,
			format,
			vk::Extent3D{ width, height, 1 },
			mipLevels,
			1,
			numSamples,
			tiling,
			usage,
			vk::SharingMode::eExclusive
		};
		m_Image = vk::raii::Image{ context.GetDevice(), imageInfo };

		vk::MemoryRequirements memoryRequirements = m_Image.getMemoryRequirements();
		vk::MemoryAllocateInfo allocateInfo{
			memoryRequirements.size,
			context.FindMemoryType(memoryRequirements.memoryTypeBits, properties)
		};
		m_Memory = vk::raii::DeviceMemory{ context.GetDevice(), allocateInfo };
		m_Image.bindMemory(*m_Memory, 0);

		vk::ImageViewCreateInfo viewInfo{
			{},
			*m_Image,
			vk::ImageViewType::e2D,
			format,
			{},
			vk::ImageSubresourceRange{ aspectFlags, 0, mipLevels, 0, 1 }
		};
		m_ImageView = vk::raii::ImageView{ context.GetDevice(), viewInfo };

		if (createSampler)
		{
			auto properties = context.GetPhysicalDevice().getProperties();
			vk::SamplerCreateInfo samplerInfo{
				{},
				vk::Filter::eLinear,
				vk::Filter::eLinear,
				vk::SamplerMipmapMode::eLinear,
				vk::SamplerAddressMode::eRepeat,
				vk::SamplerAddressMode::eRepeat,
				vk::SamplerAddressMode::eRepeat,
				0.0f,
				vk::True,
				properties.limits.maxSamplerAnisotropy,
				vk::False,
				vk::CompareOp::eAlways,
				0.0f,
				vk::LodClampNone,
				vk::BorderColor::eIntOpaqueBlack,
				vk::False
			};

			m_Sampler = vk::raii::Sampler{ context.GetDevice(), samplerInfo };
		}
	}

	Texture Texture::LoadFromFile(const VKContext& context, const std::string& filepath)
	{
		int textureWidth, textureHeight, textureChannels;
		stbi_uc* pixels = stbi_load(filepath.c_str(), &textureWidth, &textureHeight, &textureChannels, STBI_rgb_alpha);

		if (!pixels)
		{
			throw std::runtime_error("Failed to load texture image!");
		}

		uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(textureWidth, textureHeight)))) + 1;

		vk::DeviceSize imageSize = static_cast<vk::DeviceSize>(textureWidth * textureHeight * 4);
		Buffer stagingBuffer{
			context,
			imageSize,
			vk::BufferUsageFlagBits::eTransferSrc,
			vk::MemoryPropertyFlagBits::eHostVisible |
			vk::MemoryPropertyFlagBits::eHostCoherent
		};

		void* data = stagingBuffer.Map();
		memcpy(data, pixels, imageSize);
		stagingBuffer.Unmap();
		stbi_image_free(pixels);

		Texture texture{
			context,
			static_cast<uint32_t>(textureWidth),
			static_cast<uint32_t>(textureHeight),
			mipLevels,
			vk::SampleCountFlagBits::e1,
			vk::Format::eR8G8B8A8Srgb,
			vk::ImageTiling::eOptimal,
			vk::ImageUsageFlagBits::eTransferSrc |
			vk::ImageUsageFlagBits::eTransferDst |
			vk::ImageUsageFlagBits::eSampled,
			vk::MemoryPropertyFlagBits::eDeviceLocal,
			vk::ImageAspectFlagBits::eColor,
			true
		};

		transitionImageLayout(context, *texture.GetImage(), vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, mipLevels);
		copyBufferToImage(context, *stagingBuffer.GetHandle(), *texture.GetImage(), static_cast<uint32_t>(textureWidth), static_cast<uint32_t>(textureHeight));
		generateMipmaps(context, *texture.GetImage(), vk::Format::eR8G8B8A8Srgb, textureWidth, textureHeight, mipLevels);

		return texture;
	}

	Texture Texture::CreateAttachment(
		const VKContext& context,
		uint32_t width, uint32_t height,
		vk::SampleCountFlagBits numSamples,
		vk::Format format,
		vk::ImageUsageFlags usage,
		vk::ImageAspectFlags aspectFlags)
	{
		return Texture{ context, width, height, 1, numSamples, format, vk::ImageTiling::eOptimal, usage, vk::MemoryPropertyFlagBits::eDeviceLocal, aspectFlags, false };
	}

	vk::Format Texture::FindDepthFormat(const VKContext& context)
	{
		return findSupportedFormat(
			context,
			{ vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint },
			vk::ImageTiling::eOptimal,
			vk::FormatFeatureFlagBits::eDepthStencilAttachment);
	}

	void Texture::transitionImageLayout(const VKContext& context, vk::Image image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t mipLevels)
	{
		context.ExecuteImmediateCommand([&](vk::raii::CommandBuffer& commandBuffer)
			{
				vk::ImageMemoryBarrier barrier{
					{}, {},
					oldLayout,
					newLayout,
					vk::QueueFamilyIgnored,
					vk::QueueFamilyIgnored,
					image,
					vk::ImageSubresourceRange{ vk::ImageAspectFlagBits::eColor, 0, mipLevels, 0, 1}
				};

				vk::PipelineStageFlags sourceStage;
				vk::PipelineStageFlags destinationStage;

				if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal)
				{
					barrier.srcAccessMask = {};
					barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

					sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
					destinationStage = vk::PipelineStageFlagBits::eTransfer;
				}
				else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
				{
					barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
					barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

					sourceStage = vk::PipelineStageFlagBits::eTransfer;
					destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
				}
				else
				{
					throw std::runtime_error("Unsupported layer transition!");
				}

				commandBuffer.pipelineBarrier(sourceStage, destinationStage, {}, {}, nullptr, barrier);
			});
	}

	void Texture::copyBufferToImage(const VKContext& context, vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height)
	{
		context.ExecuteImmediateCommand([&](vk::raii::CommandBuffer& commandBuffer)
			{
				vk::BufferImageCopy region{
					0,
					0,
					0,
					vk::ImageSubresourceLayers{ vk::ImageAspectFlagBits::eColor, 0, 0, 1 },
					vk::Offset3D{ 0, 0, 0 },
					vk::Extent3D{ width, height, 1 }
				};
				commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, region);
			});
	}

	vk::Format Texture::findSupportedFormat(const VKContext& context, const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features)
	{
		for (const auto format : candidates)
		{
			vk::FormatProperties properties = context.GetPhysicalDevice().getFormatProperties(format);

			if (((tiling == vk::ImageTiling::eLinear) && ((properties.linearTilingFeatures & features) == features)) ||
				((tiling == vk::ImageTiling::eOptimal) && ((properties.optimalTilingFeatures & features) == features)))
			{
				return format;
			}
		}

		throw std::runtime_error("Failed to find supported format!");
	}

	void Texture::generateMipmaps(const VKContext& context, vk::Image image, vk::Format imageFormat, int32_t textureWidth, int32_t textureHeight, uint32_t mipLevels)
	{
		context.ExecuteImmediateCommand([&](vk::raii::CommandBuffer& commandBuffer)
			{
				vk::FormatProperties formatProperties = context.GetPhysicalDevice().getFormatProperties(imageFormat);
				if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear))
				{
					throw std::runtime_error("Texture image format does not support linear blitting");
				}

				vk::ImageMemoryBarrier barrier{
					vk::AccessFlagBits::eTransferWrite,
					vk::AccessFlagBits::eTransferRead,
					vk::ImageLayout::eTransferDstOptimal,
					vk::ImageLayout::eTransferSrcOptimal,
					vk::QueueFamilyIgnored,
					vk::QueueFamilyIgnored,
					image,
					vk::ImageSubresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
				};

				int32_t mipWidth = textureWidth;
				int32_t mipHeight = textureHeight;

				for (uint32_t i = 1; i < mipLevels; ++i)
				{
					barrier.subresourceRange.baseMipLevel = i - 1;
					barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
					barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
					barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
					barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
					commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, barrier);

					vk::ImageBlit blit{
						vk::ImageSubresourceLayers{ vk::ImageAspectFlagBits::eColor, i - 1, {}, 1 },
						std::array<vk::Offset3D, 2>({ {}, { mipWidth, mipHeight, 1 } }),
						vk::ImageSubresourceLayers{ vk::ImageAspectFlagBits::eColor, i, {}, 1 },
						std::array<vk::Offset3D, 2>({ {}, { 1 < mipWidth ? mipWidth / 2 : 1, 1 < mipHeight ? mipHeight / 2 : 1, 1 } })
					};
					commandBuffer.blitImage(image, vk::ImageLayout::eTransferSrcOptimal, image, vk::ImageLayout::eTransferDstOptimal, blit, vk::Filter::eLinear);

					barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
					barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
					barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
					barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
					commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, barrier);

					if (1 < mipWidth)
					{
						mipWidth /= 2;
					}
					if (1 < mipHeight)
					{
						mipHeight /= 2;
					}
				}

				barrier.subresourceRange.baseMipLevel = mipLevels - 1;
				barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
				barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
				barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
				barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
				commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, barrier);
			});
	}

}
