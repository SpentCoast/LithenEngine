#include "Graphics/Renderer.hpp"

namespace Lithen {

	Renderer::Renderer(Window& window, const VKContext& vkContext)
		: m_Window{ window }, m_Context{ vkContext }, m_Swapchain{ window, vkContext }
	{
		auto roomModel = std::make_shared<Model>(m_Context, "Models/viking_room.glb");
		auto roomTexture = std::make_shared<Texture>(Texture::LoadFromFile(m_Context, "Textures/viking_room.ktx2"));

		auto cubeModel = std::make_shared<Model>(m_Context, "Models/cube.glb");
		auto cubeTexture = std::make_shared<Texture>(Texture::LoadFromFile(m_Context, "Textures/cube.ktx2"));

		GameObject obj1{ roomModel, roomTexture };
		obj1.GetTransform().Translation = { 0.0f, 0.0f, 0.0f };
		obj1.GetTransform().Rotation = { 0.0f, 0.0f, 0.0f };
		m_GameObjects.push_back(std::move(obj1));

		GameObject obj2{ cubeModel, cubeTexture };
		obj2.GetTransform().Translation = { 2.0f, 0.0f, 0.0f };
		obj2.GetTransform().Rotation = { 0.0f, 0.0f, 0.0f };
		obj2.GetTransform().Scale = { 0.5f, 0.5f, 0.5f };
		m_GameObjects.push_back(std::move(obj2));

		createDepthResources();

		m_DescriptorManager = std::make_unique<DescriptorManager>(m_Context, MAX_FRAMES_IN_FLIGHT, m_GameObjects.size());

		std::vector<std::shared_ptr<Texture>> textures;
		for (const auto& obj : m_GameObjects)
		{
			textures.push_back(obj.GetTexture());
		}
		m_DescriptorManager->CreateObjectDescriptorSets(m_GameObjects, textures);

		createGraphicsPipeline();
		createCommandPool();
		createCommandBuffers();
		createSyncObjects();
		initImGui();
	}

	Renderer::~Renderer()
	{
		m_Context.GetDevice().waitIdle();

		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	void Renderer::DrawFrame(const Camera& camera)
	{
		auto fenceResult = m_Context.GetDevice().waitForFences(*m_InFlightFences[m_FrameIndex], vk::True, UINT64_MAX);
		if (fenceResult != vk::Result::eSuccess)
		{
			throw std::runtime_error("Failed to wait for fence!");
		}

		auto [result, imageIndex] = m_Swapchain.GetHandle().acquireNextImage(UINT64_MAX, *m_PresentCompleteSemaphores[m_FrameIndex], nullptr);

		if (result == vk::Result::eErrorOutOfDateKHR)
		{
			recreateSwapchain();
			return;
		}
		if ((result != vk::Result::eSuccess) && (result != vk::Result::eSuboptimalKHR))
		{
			assert(result == vk::Result::eTimeout || result == vk::Result::eNotReady);
			throw std::runtime_error("Failed to aquire swap chain image!");
		}

		static auto lastTime = std::chrono::high_resolution_clock::now();
		auto currentTime = std::chrono::high_resolution_clock::now();
		float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
		lastTime = currentTime;

		for (size_t i = 0; i < m_GameObjects.size(); ++i)
		{
			//m_GameObjects[i].GetTransform().Rotation.z += 0.5f * deltaTime;

			glm::mat4 modelMatrix = m_GameObjects[i].GetTransform().Mat4();
			m_DescriptorManager->UpdateUBO(m_FrameIndex, i, modelMatrix, camera);
		}

		m_Context.GetDevice().resetFences(*m_InFlightFences[m_FrameIndex]);
		m_CommandBuffers[m_FrameIndex].reset();
		recordCommandBuffer(imageIndex);

		vk::PipelineStageFlags waitDestinationStageMask{ vk::PipelineStageFlagBits::eColorAttachmentOutput };
		const vk::SubmitInfo submitInfo{
			1,
			&*m_PresentCompleteSemaphores[m_FrameIndex],
			&waitDestinationStageMask,
			1,
			&*m_CommandBuffers[m_FrameIndex],
			1,
			&*m_RenderFinishedSemaphores[imageIndex]
		};

		m_Context.GetQueue().submit(submitInfo, *m_InFlightFences[m_FrameIndex]);

		const vk::PresentInfoKHR presentInfoKHR{
			1,
			&*m_RenderFinishedSemaphores[imageIndex],
			1,
			&*m_Swapchain.GetHandle(),
			&imageIndex,
		};

		result = m_Context.GetQueue().presentKHR(presentInfoKHR);
		if ((result == vk::Result::eSuboptimalKHR) || (result == vk::Result::eErrorOutOfDateKHR) || m_Window.WasResized())
		{
			m_Window.ResetResizeFlag();
			recreateSwapchain();
		}
		else
		{
			assert(result == vk::Result::eSuccess);
		}

		m_FrameIndex = (m_FrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	void Renderer::createGraphicsPipeline()
	{
		auto bindingDescription = Vertex::GetBindingDescription();
		auto attributeDescriptions = Vertex::GetAttributeDescriptions();
		vk::PipelineVertexInputStateCreateInfo vertexInputInfo{
			{},
			1,
			&bindingDescription,
			static_cast<uint32_t>(attributeDescriptions.size()),
			attributeDescriptions.data()
		};

		m_GraphicsPipeline = std::make_unique<GraphicsPipeline>(
			m_Context,
			"Shaders/slang.spv",
			m_Context.GetMsaaSamples(),
			m_DescriptorManager->GetLayout(),
			m_Swapchain.GetFormat(),
			Texture::FindDepthFormat(m_Context),
			vertexInputInfo);
	}

	void Renderer::createCommandPool()
	{
		vk::CommandPoolCreateInfo poolInfo{ vk::CommandPoolCreateFlagBits::eResetCommandBuffer, m_Context.GetQueueIndex() };
		m_CommandPool = vk::raii::CommandPool{ m_Context.GetDevice(), poolInfo };
	}

	void Renderer::createDepthResources()
	{
		vk::Format depthFormat = Texture::FindDepthFormat(m_Context);
		vk::SampleCountFlagBits msaa = m_Context.GetMsaaSamples();

		m_ColorTexture = Texture::CreateAttachment(
			m_Context,
			m_Swapchain.GetExtent().width,
			m_Swapchain.GetExtent().height,
			msaa,
			m_Swapchain.GetFormat(),
			vk::ImageUsageFlagBits::eTransientAttachment |
			vk::ImageUsageFlagBits::eColorAttachment,
			vk::ImageAspectFlagBits::eColor);

		m_DepthTexture = Texture::CreateAttachment(
			m_Context,
			m_Swapchain.GetExtent().width,
			m_Swapchain.GetExtent().height,
			msaa,
			depthFormat,
			vk::ImageUsageFlagBits::eDepthStencilAttachment,
			vk::ImageAspectFlagBits::eDepth);
	}

	void Renderer::createCommandBuffers()
	{
		vk::CommandBufferAllocateInfo allocInfo{
			m_CommandPool,
			vk::CommandBufferLevel::ePrimary,
			MAX_FRAMES_IN_FLIGHT
		};
		m_CommandBuffers = vk::raii::CommandBuffers{ m_Context.GetDevice(), allocInfo };
	}

	void Renderer::createSyncObjects()
	{
		assert(m_PresentCompleteSemaphores.empty() && m_RenderFinishedSemaphores.empty() && m_InFlightFences.empty());

		for (size_t i = 0; i < m_Swapchain.GetImages().size(); ++i)
		{
			m_RenderFinishedSemaphores.emplace_back(m_Context.GetDevice(), vk::SemaphoreCreateInfo{});
		}

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			m_PresentCompleteSemaphores.emplace_back(m_Context.GetDevice(), vk::SemaphoreCreateInfo{});
			m_InFlightFences.emplace_back(m_Context.GetDevice(), vk::FenceCreateInfo{ vk::FenceCreateFlagBits::eSignaled });
		}
	}

	void Renderer::initImGui()
	{
		createImGuiPool();

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		ImGui::StyleColorsDark();

		ImGui_ImplGlfw_InitForVulkan(m_Window.GetWindowHandle(), true);

		ImGui_ImplVulkan_InitInfo initInfo{};
		initInfo.Instance = *m_Context.GetInstance();
		initInfo.PhysicalDevice = *m_Context.GetPhysicalDevice();
		initInfo.Device = *m_Context.GetDevice();
		initInfo.QueueFamily = m_Context.GetQueueIndex();
		initInfo.Queue = *m_Context.GetQueue();
		initInfo.PipelineCache = VK_NULL_HANDLE;
		initInfo.DescriptorPool = *m_ImGuiDescriptorPool;
		initInfo.MinImageCount = MAX_FRAMES_IN_FLIGHT;
		initInfo.ImageCount = MAX_FRAMES_IN_FLIGHT;

		initInfo.UseDynamicRendering = true;

		initInfo.PipelineInfoMain.Subpass = 0;
		initInfo.PipelineInfoMain.MSAASamples = static_cast<VkSampleCountFlagBits>(m_Context.GetMsaaSamples());

		static VkFormat colorFormat;
		colorFormat = static_cast<VkFormat>(m_Swapchain.GetFormat());

		initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
		initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.pNext = nullptr;
		initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
		initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = &colorFormat;
		initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.depthAttachmentFormat = static_cast<VkFormat>(Texture::FindDepthFormat(m_Context));

		ImGui_ImplVulkan_Init(&initInfo);
	}

	void Renderer::recreateSwapchain()
	{
		m_Swapchain.Recreate();
		createDepthResources();
	}

	std::pair<vk::raii::Image, vk::raii::DeviceMemory> Renderer::createImage(
		uint32_t width, uint32_t height,
		vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties)
	{
		vk::ImageCreateInfo imageInfo{
			{},
			vk::ImageType::e2D,
			format,
			vk::Extent3D{ width, height, 1 },
			1,
			1,
			vk::SampleCountFlagBits::e1,
			tiling,
			usage,
			vk::SharingMode::eExclusive
		};
		vk::raii::Image image = vk::raii::Image{ m_Context.GetDevice(), imageInfo };

		vk::MemoryRequirements memoryRequirements = image.getMemoryRequirements();
		vk::MemoryAllocateInfo allocateInfo{
			memoryRequirements.size,
			m_Context.FindMemoryType(memoryRequirements.memoryTypeBits, properties)
		};
		vk::raii::DeviceMemory imageMemory = vk::raii::DeviceMemory{ m_Context.GetDevice(), allocateInfo };
		image.bindMemory(imageMemory, 0);

		return { std::move(image), std::move(imageMemory) };
	}

	void Renderer::createImGuiPool()
	{
		std::array<vk::DescriptorPoolSize, 11> poolSizes = {
			vk::DescriptorPoolSize{ vk::DescriptorType::eSampler, 1000 },
			vk::DescriptorPoolSize{ vk::DescriptorType::eCombinedImageSampler, 1000 },
			vk::DescriptorPoolSize{ vk::DescriptorType::eSampledImage, 1000 },
			vk::DescriptorPoolSize{ vk::DescriptorType::eStorageImage, 1000 },
			vk::DescriptorPoolSize{ vk::DescriptorType::eUniformTexelBuffer, 1000 },
			vk::DescriptorPoolSize{ vk::DescriptorType::eStorageTexelBuffer, 1000 },
			vk::DescriptorPoolSize{ vk::DescriptorType::eUniformBuffer, 1000 },
			vk::DescriptorPoolSize{ vk::DescriptorType::eStorageBuffer, 1000 },
			vk::DescriptorPoolSize{ vk::DescriptorType::eUniformBufferDynamic, 1000 },
			vk::DescriptorPoolSize{ vk::DescriptorType::eStorageBufferDynamic, 1000 },
			vk::DescriptorPoolSize{ vk::DescriptorType::eInputAttachment, 1000 }
		};

		vk::DescriptorPoolCreateInfo poolInfo{
			vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
			1000 * 11,
			poolSizes
		};
		m_ImGuiDescriptorPool = vk::raii::DescriptorPool{ m_Context.GetDevice(), poolInfo };
	}

	void Renderer::recordCommandBuffer(uint32_t imageIndex)
	{
		auto& commandBuffer = m_CommandBuffers[m_FrameIndex];
		commandBuffer.begin({});

		transitionImageLayout(
			m_Swapchain.GetImages()[imageIndex],
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::eColorAttachmentOptimal,
			{},
			vk::AccessFlagBits2::eColorAttachmentWrite,
			vk::PipelineStageFlagBits2::eColorAttachmentOutput,
			vk::PipelineStageFlagBits2::eColorAttachmentOutput,
			vk::ImageAspectFlagBits::eColor
		);

		transitionImageLayout(
			*m_ColorTexture.GetImage(),
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::eColorAttachmentOptimal,
			vk::AccessFlagBits2::eColorAttachmentWrite,
			vk::AccessFlagBits2::eColorAttachmentWrite,
			vk::PipelineStageFlagBits2::eColorAttachmentOutput,
			vk::PipelineStageFlagBits2::eColorAttachmentOutput,
			vk::ImageAspectFlagBits::eColor
		);

		transitionImageLayout(
			*m_DepthTexture.GetImage(),
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::eDepthAttachmentOptimal,
			vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
			vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
			vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
			vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
			vk::ImageAspectFlagBits::eDepth
		);

		vk::ClearValue clearColor = vk::ClearColorValue{ 0.0f, 0.0f, 0.0f, 1.0f };
		vk::ClearValue clearDepth = vk::ClearDepthStencilValue{ 1.0f, 0 };
		vk::RenderingAttachmentInfo colorAttachment{
			*m_ColorTexture.GetImageView(),
			vk::ImageLayout::eColorAttachmentOptimal,
			vk::ResolveModeFlagBits::eAverage,
			*m_Swapchain.GetImageViews()[imageIndex],
			vk::ImageLayout::eColorAttachmentOptimal,
			vk::AttachmentLoadOp::eClear,
			vk::AttachmentStoreOp::eStore,
			clearColor
		};
		vk::RenderingAttachmentInfo depthAttachment{
			*m_DepthTexture.GetImageView(),
			vk::ImageLayout::eDepthAttachmentOptimal,
			{}, {}, {},
			vk::AttachmentLoadOp::eClear,
			vk::AttachmentStoreOp::eDontCare,
			clearDepth
		};

		vk::RenderingInfo renderingInfo{
			{},
			vk::Rect2D{ vk::Offset2D{ 0, 0 }, m_Swapchain.GetExtent() },
			1,
			{},
			1,
			&colorAttachment,
			&depthAttachment
		};

		commandBuffer.beginRendering(renderingInfo);
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *m_GraphicsPipeline->GetHandle());
		commandBuffer.setViewport(
			0,
			vk::Viewport{
				0.0f,
				0.0f,
				static_cast<float>(m_Swapchain.GetExtent().width),
				static_cast<float>(m_Swapchain.GetExtent().height),
				0.0f,
				1.0f
			});
		commandBuffer.setScissor(0, vk::Rect2D{ vk::Offset2D{0, 0}, m_Swapchain.GetExtent() });

		for (size_t i = 0; i < m_GameObjects.size(); ++i)
		{
			auto& obj = m_GameObjects[i];

			obj.GetModel()->Bind(commandBuffer);

			commandBuffer.bindDescriptorSets(
				vk::PipelineBindPoint::eGraphics,
				*m_GraphicsPipeline->GetLayout(),
				0,
				m_DescriptorManager->GetSet(i, m_FrameIndex),
				nullptr
			);

			obj.GetModel()->Draw(commandBuffer);
		}

		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), *commandBuffer);

		commandBuffer.endRendering();

		transitionImageLayout(
			m_Swapchain.GetImages()[imageIndex],
			vk::ImageLayout::eColorAttachmentOptimal,
			vk::ImageLayout::ePresentSrcKHR,
			vk::AccessFlagBits2::eColorAttachmentWrite,
			{},
			vk::PipelineStageFlagBits2::eColorAttachmentOutput,
			vk::PipelineStageFlagBits2::eBottomOfPipe,
			vk::ImageAspectFlagBits::eColor
		);

		commandBuffer.end();
	}

	void Renderer::transitionImageLayout(
		vk::Image image,
		vk::ImageLayout oldLayout,
		vk::ImageLayout newLayout,
		vk::AccessFlags2 srcAccessMask,
		vk::AccessFlags2 dstAccessMask,
		vk::PipelineStageFlags2 srcStageMask,
		vk::PipelineStageFlags2 dstStageMask,
		vk::ImageAspectFlags imageAspectFlags)
	{
		vk::ImageMemoryBarrier2 imageMemoryBarrier{
			srcStageMask,
			srcAccessMask,
			dstStageMask,
			dstAccessMask,
			oldLayout,
			newLayout,
			VK_QUEUE_FAMILY_IGNORED,
			VK_QUEUE_FAMILY_IGNORED,
			image,
			vk::ImageSubresourceRange{
				imageAspectFlags,
				0,
				1,
				0,
				1
			}
		};

		vk::DependencyInfo dependencyInfo{
			{}, {}, {}, {}, {},
			1,
			&imageMemoryBarrier
		};

		m_CommandBuffers[m_FrameIndex].pipelineBarrier2(dependencyInfo);
	}

}
