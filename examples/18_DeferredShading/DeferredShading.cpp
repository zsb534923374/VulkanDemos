﻿#include "Common/Common.h"
#include "Common/Log.h"

#include "Demo/DVKCommon.h"

#include "Math/Vector4.h"
#include "Math/Matrix4x4.h"

#include "Loader/ImageLoader.h"

#include <vector>

#define NUM_LIGHTS 64

class DeferredShadingDemo : public DemoBase
{
public:
	DeferredShadingDemo(int32 width, int32 height, const char* title, const std::vector<std::string>& cmdLine)
		: DemoBase(width, height, title, cmdLine)
	{
        
	}
    
	virtual ~DeferredShadingDemo()
	{

	}

	virtual bool PreInit() override
	{
		return true;
	}

	virtual bool Init() override
	{
		DemoBase::Setup();
		DemoBase::Prepare();

		LoadAssets();
		CreateTexture();
		CreateGUI();
		CreateUniformBuffers();
        CreateDescriptorSet();
		CreatePipelines();
		SetupCommandBuffers();

		m_Ready = true;

		return true;
	}

	virtual void Exist() override
	{
		DemoBase::Release();

		DestroyAssets();
		DestroyGUI();
		DestroyPipelines();
		DestroyUniformBuffers();

		DestroyAttachments();
	}

	virtual void Loop(float time, float delta) override
	{
		if (!m_Ready) {
			return;
		}
		Draw(time, delta);
	}

protected:

	void CreateFrameBuffers() override
	{
		DestroyFrameBuffers();
		
		int32 fwidth    = GetVulkanRHI()->GetSwapChain()->GetWidth();
		int32 fheight   = GetVulkanRHI()->GetSwapChain()->GetHeight();
		VkDevice device = GetVulkanRHI()->GetDevice()->GetInstanceHandle();

		VkImageView attachments[5];

		VkFramebufferCreateInfo frameBufferCreateInfo;
		ZeroVulkanStruct(frameBufferCreateInfo, VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO);
		frameBufferCreateInfo.renderPass      = m_RenderPass;
		frameBufferCreateInfo.attachmentCount = 5;
		frameBufferCreateInfo.pAttachments    = attachments;
		frameBufferCreateInfo.width			  = fwidth;
		frameBufferCreateInfo.height		  = fheight;
		frameBufferCreateInfo.layers		  = 1;

		const std::vector<VkImageView>& backbufferViews = GetVulkanRHI()->GetBackbufferViews();

		m_FrameBuffers.resize(backbufferViews.size());
		for (uint32 i = 0; i < m_FrameBuffers.size(); ++i) {
			attachments[0] = backbufferViews[i];
			attachments[1] = m_AttachsColor[i]->imageView;
            attachments[2] = m_AttachsNormal[i]->imageView;
			attachments[3] = m_AttachsPosition[i]->imageView;
			attachments[4] = m_AttachsDepth[i]->imageView;
			VERIFYVULKANRESULT(vkCreateFramebuffer(device, &frameBufferCreateInfo, VULKAN_CPU_ALLOCATOR, &m_FrameBuffers[i]));
		}


		//创建Debug的RENDERPASS
		VkImageView attachmentsDebug[2];

		VkFramebufferCreateInfo frameBufferCreateInfoDebug;
		ZeroVulkanStruct(frameBufferCreateInfoDebug, VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO);
		frameBufferCreateInfoDebug.renderPass = m_RenderPassDebug;
		frameBufferCreateInfoDebug.attachmentCount = 2;
		frameBufferCreateInfoDebug.pAttachments = attachmentsDebug;
		frameBufferCreateInfoDebug.width = fwidth;
		frameBufferCreateInfoDebug.height = fheight;
		frameBufferCreateInfoDebug.layers = 1;

//		const std::vector<VkImageView>& backbufferViews = GetVulkanRHI()->GetBackbufferViews();

		m_FrameBuffersDebug.resize(backbufferViews.size());
		for (uint32 i = 0; i < m_FrameBuffersDebug.size(); ++i) {
			attachmentsDebug[0] = backbufferViews[i];
			attachmentsDebug[1] = m_AttachsDepth[i]->imageView;
			VERIFYVULKANRESULT(vkCreateFramebuffer(device, &frameBufferCreateInfoDebug, VULKAN_CPU_ALLOCATOR, &m_FrameBuffersDebug[i]));
		}
	}

	void CreateDepthStencil() override
	{

	}

	void DestoryDepthStencil() override
	{
		
	}

	void DestroyAttachments()
	{
		for (int32 i = 0; i < m_AttachsDepth.size(); ++i) 
		{
			vk_demo::DVKTexture* texture = m_AttachsDepth[i];
			delete texture;
		}
		m_AttachsDepth.clear();

		for (int32 i = 0; i < m_AttachsColor.size(); ++i) 
		{
			vk_demo::DVKTexture* texture = m_AttachsColor[i];
			delete texture;
		}
		m_AttachsColor.clear();
        
        for (int32 i = 0; i < m_AttachsNormal.size(); ++i)
        {
            vk_demo::DVKTexture* texture = m_AttachsNormal[i];
            delete texture;
        }
        m_AttachsNormal.clear();

		for (int32 i = 0; i < m_AttachsPosition.size(); ++i)
		{
			vk_demo::DVKTexture* texture = m_AttachsPosition[i];
			delete texture;
		}
		m_AttachsPosition.clear();
	}

	void CreateAttachments()
	{
		auto swapChain  = GetVulkanRHI()->GetSwapChain();
		int32 fwidth    = swapChain->GetWidth();
		int32 fheight   = swapChain->GetHeight();
		int32 numBuffer = swapChain->GetBackBufferCount();
		
		m_AttachsColor.resize(numBuffer);
        m_AttachsNormal.resize(numBuffer);
		m_AttachsPosition.resize(numBuffer);
		m_AttachsDepth.resize(numBuffer);
		m_AttachsDebug.resize(numBuffer);

		for (int32 i = 0; i < m_AttachsColor.size(); ++i)
		{
			m_AttachsColor[i] = vk_demo::DVKTexture::CreateAttachment(
				m_VulkanDevice,
				PixelFormatToVkFormat(GetVulkanRHI()->GetPixelFormat(), false), 
				VK_IMAGE_ASPECT_COLOR_BIT,
				fwidth, fheight,
				VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT
			);
		}
        
        for (int32 i = 0; i < m_AttachsNormal.size(); ++i)
        {
            m_AttachsNormal[i] = vk_demo::DVKTexture::CreateAttachment(
                m_VulkanDevice,
				VK_FORMAT_R16G16B16A16_SFLOAT,
                VK_IMAGE_ASPECT_COLOR_BIT,
                fwidth, fheight,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
            );
        }

		for (int32 i = 0; i < m_AttachsPosition.size(); ++i)
		{
			m_AttachsPosition[i] = vk_demo::DVKTexture::CreateAttachment(
				m_VulkanDevice,
				VK_FORMAT_R16G16B16A16_SFLOAT,
				VK_IMAGE_ASPECT_COLOR_BIT,
				fwidth, fheight,
				VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT
			);
		}
        
		for (int32 i = 0; i < m_AttachsDepth.size(); ++i)
		{
			m_AttachsDepth[i] = vk_demo::DVKTexture::CreateAttachment(
				m_VulkanDevice,
				PixelFormatToVkFormat(m_DepthFormat, false), 
				VK_IMAGE_ASPECT_DEPTH_BIT,
				fwidth, fheight,
				VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT
			);
		}
	}

	void CreateRenderPass() override
	{
		DestoryRenderPass();
		DestroyAttachments();
		CreateAttachments();

		VkDevice device = GetVulkanRHI()->GetDevice()->GetInstanceHandle();
		PixelFormat pixelFormat = GetVulkanRHI()->GetPixelFormat();

		std::vector<VkAttachmentDescription> attachments(5);
		// swap chain attachment
		attachments[0].format		  = PixelFormatToVkFormat(pixelFormat, false);
		attachments[0].samples		  = m_SampleCount;
		attachments[0].loadOp		  = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[0].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[0].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[0].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[0].finalLayout	  = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		// color attachment
		attachments[1].format         = PixelFormatToVkFormat(pixelFormat, false);
		attachments[1].samples        = m_SampleCount;
		attachments[1].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[1].storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[1].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[1].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[1].finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        // normal attachment
        attachments[2].format         = m_AttachsNormal[0]->format;
        attachments[2].samples        = m_SampleCount;
        attachments[2].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[2].storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[2].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[2].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[2].finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		// position
		attachments[3].format         = m_AttachsPosition[0]->format;
		attachments[3].samples        = m_SampleCount;
		attachments[3].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[3].storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[3].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[3].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[3].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[3].finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		// depth stencil attachment
		attachments[4].format         = PixelFormatToVkFormat(m_DepthFormat, false);
		attachments[4].samples        = m_SampleCount;
		attachments[4].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[4].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[4].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[4].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[4].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[4].finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		
		VkAttachmentReference colorReferences[3];
		colorReferences[0].attachment = 1;
		colorReferences[0].layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorReferences[1].attachment = 2;
        colorReferences[1].layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colorReferences[2].attachment = 3;
		colorReferences[2].layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		
		VkAttachmentReference swapReference = { };
		swapReference.attachment = 0;
		swapReference.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthReference = { };
		depthReference.attachment = 4;
		depthReference.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        
		VkAttachmentReference inputReferences[4];
		inputReferences[0].attachment = 1;
		inputReferences[0].layout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        inputReferences[1].attachment = 2;
        inputReferences[1].layout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		inputReferences[2].attachment = 3;
		inputReferences[2].layout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		inputReferences[3].attachment = 4;
		inputReferences[3].layout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		//指定color和深度
		std::vector<VkSubpassDescription> subpassDescriptions(2);
		subpassDescriptions[0].pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescriptions[0].colorAttachmentCount    = 3;
		subpassDescriptions[0].pColorAttachments       = colorReferences;     //可以看到第一个pass中输出到这个colorReferences附着中   所以第一个管线应该要绑定3个RT和一个深度缓冲
		subpassDescriptions[0].pDepthStencilAttachment = &depthReference;     //深度输出到depth附着中   
        
		//指定color和输入
		subpassDescriptions[1].pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescriptions[1].colorAttachmentCount    = 1;
		subpassDescriptions[1].pColorAttachments       = &swapReference;
		subpassDescriptions[1].inputAttachmentCount    = 4;
		subpassDescriptions[1].pInputAttachments       = inputReferences;    //他需要4个输入，这4个输入引用的是inputReferences  （刚好这4个输入分别是第一个pass输出的color附着和depth附着）
		
        std::vector<VkSubpassDependency> dependencies(3);
        dependencies[0].srcSubpass      = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass      = 0;
        dependencies[0].srcStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependencies[0].dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[0].srcAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
        dependencies[0].dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        
        dependencies[1].srcSubpass      = 0;
        dependencies[1].dstSubpass      = 1;
        dependencies[1].srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[1].dstStageMask    = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[1].srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[1].dstAccessMask   = VK_ACCESS_SHADER_READ_BIT;
        dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        dependencies[2].srcSubpass      = 1;
        dependencies[2].dstSubpass      = VK_SUBPASS_EXTERNAL;
        dependencies[2].srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[2].dstStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependencies[2].srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[2].dstAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
        dependencies[2].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
		
		VkRenderPassCreateInfo renderPassInfo;
		ZeroVulkanStruct(renderPassInfo, VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO);
		renderPassInfo.attachmentCount = attachments.size();
		renderPassInfo.pAttachments    = attachments.data();
		renderPassInfo.subpassCount    = subpassDescriptions.size();
		renderPassInfo.pSubpasses      = subpassDescriptions.data();
		renderPassInfo.dependencyCount = dependencies.size();
		renderPassInfo.pDependencies   = dependencies.data();
		VERIFYVULKANRESULT(vkCreateRenderPass(device, &renderPassInfo, VULKAN_CPU_ALLOCATOR, &m_RenderPass));


		//这个renderpass和创建的新资源没有关系。。
		//创建debugRenderPass
		std::vector<VkAttachmentDescription> attachmentsDebug(2);
		// swap chain attachment
		attachmentsDebug[0].format = PixelFormatToVkFormat(pixelFormat, false);
		attachmentsDebug[0].samples = m_SampleCount;
		attachmentsDebug[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachmentsDebug[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachmentsDebug[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentsDebug[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentsDebug[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachmentsDebug[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	
		// depth stencil attachment
		attachmentsDebug[1].format = m_AttachsDepth[0]->format;
		attachmentsDebug[1].samples = m_SampleCount;
		attachmentsDebug[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachmentsDebug[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachmentsDebug[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachmentsDebug[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachmentsDebug[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachmentsDebug[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference swapReferenceDebug;
		swapReferenceDebug.attachment = 0;
		swapReferenceDebug.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthReferenceDebug;
		depthReferenceDebug.attachment = 1;
		depthReferenceDebug.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


		//debugRenderPass的子流程
		VkSubpassDescription subpassDescriptionsDebug;
		memset(&subpassDescriptionsDebug, 0, sizeof(VkSubpassDescription));

		subpassDescriptionsDebug.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescriptionsDebug.colorAttachmentCount = 1;
		subpassDescriptionsDebug.pColorAttachments = &swapReferenceDebug;     //可以看到第一个pass中输出到这个colorReferences附着中   所以第一个管线应该要绑定3个RT和一个深度缓冲
		subpassDescriptionsDebug.pDepthStencilAttachment = &depthReferenceDebug;     //深度输出到depth附着中 

		std::vector<VkSubpassDependency> dependenciesDebug(2);
		dependenciesDebug[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependenciesDebug[0].dstSubpass = 0;
		dependenciesDebug[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependenciesDebug[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependenciesDebug[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		dependenciesDebug[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependenciesDebug[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		dependenciesDebug[1].srcSubpass = 0;
		dependenciesDebug[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependenciesDebug[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependenciesDebug[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependenciesDebug[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependenciesDebug[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		dependenciesDebug[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		VkRenderPassCreateInfo renderPassInfoDebug;
		ZeroVulkanStruct(renderPassInfoDebug, VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO);
		renderPassInfoDebug.attachmentCount = attachmentsDebug.size();
		renderPassInfoDebug.pAttachments = attachmentsDebug.data();
		renderPassInfoDebug.subpassCount = 1;
		renderPassInfoDebug.pSubpasses = &subpassDescriptionsDebug;
		renderPassInfoDebug.dependencyCount = 2;
		renderPassInfoDebug.pDependencies = dependenciesDebug.data();
		VERIFYVULKANRESULT(vkCreateRenderPass(device, &renderPassInfoDebug, VULKAN_CPU_ALLOCATOR, &m_RenderPassDebug));
	}

	void DestroyFrameBuffers() override
	{
		VkDevice device = GetVulkanRHI()->GetDevice()->GetInstanceHandle();
		for (int32 i = 0; i < m_FrameBuffers.size(); ++i) {
			vkDestroyFramebuffer(device, m_FrameBuffers[i], VULKAN_CPU_ALLOCATOR);
		}
		m_FrameBuffers.clear();
	}

	void DestoryRenderPass() override
	{
		VkDevice device = GetVulkanRHI()->GetDevice()->GetInstanceHandle();
		if (m_RenderPass != VK_NULL_HANDLE) {
			vkDestroyRenderPass(device, m_RenderPass, VULKAN_CPU_ALLOCATOR);
			m_RenderPass = VK_NULL_HANDLE;
		}

		if (m_RenderPassDebug != VK_NULL_HANDLE) {
			vkDestroyRenderPass(device, m_RenderPassDebug, VULKAN_CPU_ALLOCATOR);
			m_RenderPassDebug = VK_NULL_HANDLE;
		}
	}

private:

	struct ModelBlock
	{
		Matrix4x4 model;
	};

	struct ViewProjectionBlock
	{
		Matrix4x4 view;
		Matrix4x4 projection;
	};

	struct PointLight
	{
		Vector4 position;
		Vector3 color;
		float	radius;
	};

	struct LightSpawnBlock
	{
		Vector3 position[NUM_LIGHTS];
		Vector3 direction[NUM_LIGHTS];
		float speed[NUM_LIGHTS];
	};

	struct LightDataBlock
	{
		PointLight lights[NUM_LIGHTS];
	};
    
	void Draw(float time, float delta)
	{
		int32 bufferIndex = DemoBase::AcquireBackbufferIndex();
		
		bool hovered = UpdateUI(time, delta);
		if (!hovered) {
			m_ViewCamera.Update(time, delta);
		}

		UpdateUniform(time, delta);
		
		DemoBase::Present(bufferIndex);
	}

	void UpdateUniform(float time, float delta)
	{
		m_ViewProjData.view = m_ViewCamera.GetView();
		m_ViewProjData.projection = m_ViewCamera.GetProjection();
		m_ViewProjBuffer->CopyFrom(&m_ViewProjData, sizeof(ViewProjectionBlock));

		for (int32 i = 0; i < NUM_LIGHTS; ++i)
		{
			float bias = MMath::Sin(time * m_LightInfos.speed[i]) / 5.0f;
			m_LightDatas.lights[i].position.x = m_LightInfos.position[i].x + bias * m_LightInfos.direction[i].x * 500.0f;
			m_LightDatas.lights[i].position.y = m_LightInfos.position[i].y + bias * m_LightInfos.direction[i].y * 500.0f;
			m_LightDatas.lights[i].position.z = m_LightInfos.position[i].z + bias * m_LightInfos.direction[i].z * 500.0f;
		}
		m_LightBuffer->CopyFrom(&m_LightDatas, sizeof(LightDataBlock));
	}
    
	bool UpdateUI(float time, float delta)
	{
		m_GUI->StartFrame();
        
		{
			ImGui::SetNextWindowPos(ImVec2(0, 0));
			ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiSetCond_FirstUseEver);
            ImGui::Begin("DeferredShadingDemo", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
			ImGui::Text("Simple Deferred");

			if (ImGui::Button("Random")) 
			{
				vk_demo::DVKBoundingBox bounds = m_Model->rootNode->GetBounds();
				Vector3 boundSize   = bounds.max - bounds.min;
				Vector3 boundCenter = bounds.min + boundSize * 0.5f;

				for (int32 i = 0; i < NUM_LIGHTS; ++i)
				{
					m_LightDatas.lights[i].position.x = MMath::RandRange(bounds.min.x, bounds.max.x);
					m_LightDatas.lights[i].position.y = MMath::RandRange(bounds.min.y, bounds.max.y);
					m_LightDatas.lights[i].position.z = MMath::RandRange(bounds.min.z, bounds.max.z);

					m_LightDatas.lights[i].color.x = MMath::RandRange(0.0f, 1.0f);
					m_LightDatas.lights[i].color.y = MMath::RandRange(0.0f, 1.0f);
					m_LightDatas.lights[i].color.z = MMath::RandRange(0.0f, 1.0f);

					m_LightDatas.lights[i].radius = MMath::RandRange(50.0f, 200.0f);

					m_LightInfos.position[i]  = m_LightDatas.lights[i].position;
					m_LightInfos.direction[i] = m_LightInfos.position[i];
					m_LightInfos.direction[i].Normalize();
					m_LightInfos.speed[i] = 1.0f + MMath::RandRange(0.0f, 5.0f);
				}
			}
            
			ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::End();
		}
        
		bool hovered = ImGui::IsAnyWindowHovered() || ImGui::IsAnyItemHovered() || ImGui::IsRootWindowOrAnyChildHovered();

		m_GUI->EndFrame();
		if (m_GUI->Update()) {
			SetupCommandBuffers();
		}

		return hovered;
	}

	void LoadAssets()
	{
		m_Shader0 = vk_demo::DVKShader::Create(
			m_VulkanDevice, 
			"assets/shaders/18_DeferredShading/obj.vert.spv",
			"assets/shaders/18_DeferredShading/obj.frag.spv"
		);

		m_Shader1 = vk_demo::DVKShader::Create(
			m_VulkanDevice,
			"assets/shaders/18_DeferredShading/quad.vert.spv",
			"assets/shaders/18_DeferredShading/quad.frag.spv"
		);

		m_Shader2 = vk_demo::DVKShader::Create(
			m_VulkanDevice,
			"assets/shaders/18_DeferredShading/debugquad.vert.spv",
			"assets/shaders/18_DeferredShading/debugquad.frag.spv"
		);

		vk_demo::DVKCommandBuffer* cmdBuffer = vk_demo::DVKCommandBuffer::Create(m_VulkanDevice, m_CommandPool);

		// scene model
		m_Model = vk_demo::DVKModel::LoadFromFile(
			"assets/models/Room/miniHouse_FBX.FBX",
			m_VulkanDevice,
			cmdBuffer,
			m_Shader0->perVertexAttributes
		);
        
		// quad model
        std::vector<float> vertices = {
            -1.0f,  1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 0.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 1.0f
        };
        std::vector<uint16> indices = {
            0, 1, 2, 0, 2, 3
        };
        
        m_Quad = vk_demo::DVKModel::Create(
            m_VulkanDevice,
            cmdBuffer,
            vertices,
            indices,
            m_Shader1->perVertexAttributes
        );

		// debug quad model
		std::vector<float> debugvertices = {
			-1.0f,  -0.2f, 0.0f, 0.0f, 0.0f,
			-0.2f,  -0.2f, 0.0f, 1.0f, 0.0f,
			-0.2f, -1.0f, 0.0f, 1.0f, 1.0f,
			-1.0f, -1.0f, 0.0f, 0.0f, 1.0f
		};
		std::vector<uint16> debugindices = {
			0, 1, 2, 0, 2, 3
		};

		m_debugQuad = vk_demo::DVKModel::Create(
			m_VulkanDevice,
			cmdBuffer,
			debugvertices,
			debugindices,
			m_Shader2->perVertexAttributes
		);

		delete cmdBuffer;
	}
    
	void CreateTexture()
	{
		m_AttachsDebug.resize(3);
		vk_demo::DVKCommandBuffer* cmdBuffer = vk_demo::DVKCommandBuffer::Create(m_VulkanDevice, m_CommandPool);
		for (int32 i = 0; i < m_AttachsDebug.size(); ++i)
		{
			m_AttachsDebug[i] = vk_demo::DVKTexture::Create2D("assets/textures/head_normal.jpg", m_VulkanDevice, cmdBuffer);
		}
	}

	void DestroyAssets()
	{
		delete m_Model;
        delete m_Quad;
		delete m_debugQuad;

		delete m_Shader0;
		delete m_Shader1;
		delete m_Shader2;
	}
    
	void SetupCommandBuffers()
	{
		VkCommandBufferBeginInfo cmdBeginInfo;
		ZeroVulkanStruct(cmdBeginInfo, VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO);

		VkClearValue clearValues[5];
		clearValues[0].color        = { { 0.2f, 0.2f, 0.2f, 0.0f } };
		clearValues[1].color        = { { 0.2f, 0.2f, 0.2f, 0.0f } };
        clearValues[2].color        = { { 0.0f, 0.0f, 0.0f, 0.0f } };
		clearValues[3].color		= { { 0.0f, 0.0f, 0.0f, 0.0f } };
		clearValues[4].depthStencil = { 1.0f, 0 };


		VkClearValue clearValuesDebug[2];
		clearValuesDebug[0].color = { { 0.2f, 0.2f, 0.2f, 0.0f } };
		clearValuesDebug[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo renderPassBeginInfo;
		ZeroVulkanStruct(renderPassBeginInfo, VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO);
        renderPassBeginInfo.renderPass      = m_RenderPass;
		renderPassBeginInfo.clearValueCount = 5;
		renderPassBeginInfo.pClearValues    = clearValues;
		renderPassBeginInfo.renderArea.offset.x = 0;
		renderPassBeginInfo.renderArea.offset.y = 0;
        renderPassBeginInfo.renderArea.extent.width  = m_FrameWidth;
        renderPassBeginInfo.renderArea.extent.height = m_FrameHeight;
        
		VkViewport viewport = {};
        viewport.x        = 0;
        viewport.y        = m_FrameHeight;
        viewport.width    = m_FrameWidth;
        viewport.height   = -(float)m_FrameHeight;    // flip y axis
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
		
        VkRect2D scissor = {};
        scissor.extent.width  = m_FrameWidth;
        scissor.extent.height = m_FrameHeight;
        scissor.offset.x      = 0;
        scissor.offset.y      = 0;

		uint32 alignment  = m_VulkanDevice->GetLimits().minUniformBufferOffsetAlignment;
		uint32 modelAlign = Align(sizeof(ModelBlock), alignment);

		for (int32 i = 0; i < m_CommandBuffers.size(); ++i)
		{
            renderPassBeginInfo.framebuffer = m_FrameBuffers[i];
            
			VERIFYVULKANRESULT(vkBeginCommandBuffer(m_CommandBuffers[i], &cmdBeginInfo));
			vkCmdBeginRenderPass(m_CommandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			vkCmdSetViewport(m_CommandBuffers[i], 0, 1, &viewport);
			vkCmdSetScissor(m_CommandBuffers[i],  0, 1, &scissor);

			// pass0
			//这个pass用来画gbuffer
			{
				vkCmdBindPipeline(m_CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline0->pipeline);
				for (int32 meshIndex = 0; meshIndex < m_Model->meshes.size(); ++meshIndex) {
					uint32 offset = meshIndex * modelAlign;
					vkCmdBindDescriptorSets(m_CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline0->pipelineLayout, 0, m_DescriptorSet0->descriptorSets.size(), m_DescriptorSet0->descriptorSets.data(), 1, &offset);
					m_Model->meshes[meshIndex]->BindDrawCmd(m_CommandBuffers[i]);   //绑定顶点到管线
				}
			}

			vkCmdNextSubpass(m_CommandBuffers[i], VK_SUBPASS_CONTENTS_INLINE);

			// pass1
			//这个pass把gbuffer当做输入，画在一张全屏上
			{
				vkCmdBindPipeline(m_CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline1->pipeline);
				vkCmdBindDescriptorSets(m_CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline1->pipelineLayout, 0, m_DescriptorSets[i]->descriptorSets.size(), m_DescriptorSets[i]->descriptorSets.data(), 0, nullptr);
                for (int32 meshIndex = 0; meshIndex < m_Quad->meshes.size(); ++meshIndex) {
                    m_Quad->meshes[meshIndex]->BindDrawCmd(m_CommandBuffers[i]);
                }
			}

			vkCmdEndRenderPass(m_CommandBuffers[i]);


			//用第二个pass画debug信息
//			vkCmdNextSubpass(m_CommandBuffers[i], VK_SUBPASS_CONTENTS_INLINE);

			VkRenderPassBeginInfo renderPassBeginInfo;
			ZeroVulkanStruct(renderPassBeginInfo, VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO);
			renderPassBeginInfo.renderPass = m_RenderPassDebug;
			renderPassBeginInfo.clearValueCount = 2;
			renderPassBeginInfo.pClearValues = clearValuesDebug;
			renderPassBeginInfo.renderArea.offset.x = 0;
			renderPassBeginInfo.renderArea.offset.y = 0;
			renderPassBeginInfo.renderArea.extent.width = m_FrameWidth;
			renderPassBeginInfo.renderArea.extent.height = m_FrameHeight;
			renderPassBeginInfo.framebuffer = m_FrameBuffersDebug[i];

			// pass2
			//这个pass把gbuffer当做输入，画在一张全屏上
			{

//				VERIFYVULKANRESULT(vkBeginCommandBuffer(m_CommandBuffers[i], &cmdBeginInfo));
				vkCmdBeginRenderPass(m_CommandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

				vkCmdBindPipeline(m_CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline2->pipeline);
				vkCmdBindDescriptorSets(m_CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline2->pipelineLayout, 0, m_DebugDescriptorSets[i]->descriptorSets.size(), m_DebugDescriptorSets[i]->descriptorSets.data(), 0, nullptr);
				for (int32 meshIndex = 0; meshIndex < m_debugQuad->meshes.size(); ++meshIndex) {
					m_debugQuad->meshes[meshIndex]->BindDrawCmd(m_CommandBuffers[i]);
				}
			}
            
//			m_GUI->BindDrawCmd(m_CommandBuffers[i], m_RenderPass, 1);

			vkCmdEndRenderPass(m_CommandBuffers[i]);
			VERIFYVULKANRESULT(vkEndCommandBuffer(m_CommandBuffers[i]));
		}
	}
    
	void CreateDescriptorSet()
	{
		m_DescriptorSet0 = m_Shader0->AllocateDescriptorSet();
		m_DescriptorSet0->WriteBuffer("uboViewProj", m_ViewProjBuffer);
		m_DescriptorSet0->WriteBuffer("uboModel",    m_ModelBuffer);

		m_DescriptorSets.resize(m_AttachsColor.size());
		for (int32 i = 0; i < m_DescriptorSets.size(); ++i)
		{
			m_DescriptorSets[i] = m_Shader1->AllocateDescriptorSet();
			m_DescriptorSets[i]->WriteImage("inputColor", m_AttachsColor[i]);
            m_DescriptorSets[i]->WriteImage("inputNormal", m_AttachsNormal[i]);
			m_DescriptorSets[i]->WriteImage("inputDepth", m_AttachsDepth[i]);
			m_DescriptorSets[i]->WriteImage("inputPosition", m_AttachsPosition[i]);
			m_DescriptorSets[i]->WriteBuffer("lightDatas", m_LightBuffer);
		}

		//没有采样器？
		m_DebugDescriptorSets.resize(m_AttachsDebug.size());
		for (int32 i = 0; i < m_DebugDescriptorSets.size(); ++i)
		{
			m_DebugDescriptorSets[i] = m_Shader2->AllocateDescriptorSet();
			m_DebugDescriptorSets[i]->WriteImage("inputNormal", m_AttachsDebug[i]);
		}


	}
    
	void CreatePipelines()
	{
		//创建第一个pipeline，只是用于画gbuffer
		vk_demo::DVKGfxPipelineInfo pipelineInfo0;
		pipelineInfo0.shader = m_Shader0;
        pipelineInfo0.colorAttachmentCount = 3;
		m_Pipeline0 = vk_demo::DVKGfxPipeline::Create(
			m_VulkanDevice, 
			m_PipelineCache, 
			pipelineInfo0, 
			{ 
				m_Model->GetInputBinding()
			}, 
			m_Model->GetInputAttributes(), 
			m_Shader0->pipelineLayout, 
			m_RenderPass
		);
		
		//第二个pipeline，用于画全屏
		vk_demo::DVKGfxPipelineInfo pipelineInfo1;
		pipelineInfo1.depthStencilState.depthTestEnable   = VK_FALSE;
		pipelineInfo1.depthStencilState.depthWriteEnable  = VK_FALSE;
		pipelineInfo1.depthStencilState.stencilTestEnable = VK_FALSE;
		pipelineInfo1.shader  = m_Shader1;
		pipelineInfo1.subpass = 1;
		m_Pipeline1 = vk_demo::DVKGfxPipeline::Create(
			m_VulkanDevice, 
			m_PipelineCache, 
			pipelineInfo1, 
			{ 
				m_Quad->GetInputBinding()
			}, 
			m_Quad->GetInputAttributes(), 
			m_Shader1->pipelineLayout, 
			m_RenderPass
		);

		//画gbuffer
		vk_demo::DVKGfxPipelineInfo pipelineInfo2;
		pipelineInfo2.depthStencilState.depthTestEnable = VK_FALSE;
		pipelineInfo2.depthStencilState.depthWriteEnable = VK_FALSE;
		pipelineInfo2.depthStencilState.stencilTestEnable = VK_FALSE;
		pipelineInfo2.shader = m_Shader2;
//		pipelineInfo2.subpass = 2;
		m_Pipeline2 = vk_demo::DVKGfxPipeline::Create(
			m_VulkanDevice,
			m_PipelineCache,
			pipelineInfo2,
			{
				m_debugQuad->GetInputBinding()
			},
			m_debugQuad->GetInputAttributes(),
			m_Shader2->pipelineLayout,
			m_RenderPass
		);


	}
    
	void DestroyPipelines()
	{
        delete m_Pipeline0;
		delete m_Pipeline1;

		delete m_DescriptorSet0;
		for (int32 i = 0; i < m_DescriptorSets.size(); ++i)
		{
			vk_demo::DVKDescriptorSet* descriptorSet = m_DescriptorSets[i];
			delete descriptorSet;
		}
		m_DescriptorSets.clear();
	}
	
	void CreateUniformBuffers()
	{
		vk_demo::DVKBoundingBox bounds = m_Model->rootNode->GetBounds();
		Vector3 boundSize   = bounds.max - bounds.min;
		Vector3 boundCenter = bounds.min + boundSize * 0.5f;

		// dynamic
		uint32 alignment  = m_VulkanDevice->GetLimits().minUniformBufferOffsetAlignment;
		uint32 modelAlign = Align(sizeof(ModelBlock), alignment);
        m_ModelDatas.resize(modelAlign * m_Model->meshes.size());
        for (int32 i = 0; i < m_Model->meshes.size(); ++i)
        {
            ModelBlock* modelBlock = (ModelBlock*)(m_ModelDatas.data() + modelAlign * i);
            modelBlock->model = m_Model->meshes[i]->linkNode->GetGlobalMatrix();
        }
        
		m_ModelBuffer = vk_demo::DVKBuffer::CreateBuffer(
			m_VulkanDevice, 
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
			m_ModelDatas.size(),
			m_ModelDatas.data()
		);
		m_ModelBuffer->Map();
        
		// light datas
		for (int32 i = 0; i < NUM_LIGHTS; ++i)
		{
			m_LightDatas.lights[i].position.x = MMath::RandRange(bounds.min.x, bounds.max.x);
			m_LightDatas.lights[i].position.y = MMath::RandRange(bounds.min.y, bounds.max.y);
			m_LightDatas.lights[i].position.z = MMath::RandRange(bounds.min.z, bounds.max.z);

			m_LightDatas.lights[i].color.x = MMath::RandRange(0.0f, 1.0f);
			m_LightDatas.lights[i].color.y = MMath::RandRange(0.0f, 1.0f);
			m_LightDatas.lights[i].color.z = MMath::RandRange(0.0f, 1.0f);

			m_LightDatas.lights[i].radius = MMath::RandRange(50.0f, 200.0f);

			m_LightInfos.position[i]  = m_LightDatas.lights[i].position;
			m_LightInfos.direction[i] = m_LightInfos.position[i];
			m_LightInfos.direction[i].Normalize();
			m_LightInfos.speed[i] = 1.0f + MMath::RandRange(0.0f, 5.0f);
		}
		m_LightBuffer = vk_demo::DVKBuffer::CreateBuffer(
			m_VulkanDevice,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			sizeof(LightDataBlock),
			&(m_LightDatas)
		);
		m_LightBuffer->Map();

		m_ViewProjBuffer = vk_demo::DVKBuffer::CreateBuffer(
			m_VulkanDevice, 
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
			sizeof(ViewProjectionBlock),
			&(m_ViewProjData)
		);
		m_ViewProjBuffer->Map();

		m_ViewCamera.Perspective(PI / 4, GetWidth(), GetHeight(), 300.0f, 3000.0f);
		m_ViewCamera.SetPosition(boundCenter.x, boundCenter.y + 1000, boundCenter.z - boundSize.Size());
		m_ViewCamera.LookAt(boundCenter);
	}
	
	void DestroyUniformBuffers()
	{
		m_ViewProjBuffer->UnMap();
		delete m_ViewProjBuffer;
		m_ViewProjBuffer = nullptr;

		m_ModelBuffer->UnMap();
		delete m_ModelBuffer;
		m_ModelBuffer = nullptr;

		m_LightBuffer->UnMap();
		delete m_LightBuffer;
		m_LightBuffer = nullptr;
	}

	void CreateGUI()
	{
		m_GUI = new ImageGUIContext();
		m_GUI->Init("assets/fonts/Ubuntu-Regular.ttf");
	}

	void DestroyGUI()
	{
		m_GUI->Destroy();
		delete m_GUI;
	}

private:

	typedef std::vector<vk_demo::DVKDescriptorSet*>		DVKDescriptorSetArray;
	typedef std::vector<vk_demo::DVKTexture*>			DVKTextureArray;

	bool 							m_Ready = false;
    
	vk_demo::DVKCamera				m_ViewCamera;

	std::vector<uint8>              m_ModelDatas;
	vk_demo::DVKBuffer*				m_ModelBuffer = nullptr;

	vk_demo::DVKBuffer*				m_ViewProjBuffer = nullptr;
	ViewProjectionBlock				m_ViewProjData;

	vk_demo::DVKBuffer*				m_LightBuffer = nullptr;
	LightDataBlock					m_LightDatas;
	LightSpawnBlock					m_LightInfos;
    
	vk_demo::DVKModel*				m_Model = nullptr;
    vk_demo::DVKModel*              m_Quad = nullptr;
	vk_demo::DVKModel*				m_debugQuad = nullptr;

    vk_demo::DVKGfxPipeline*        m_Pipeline0 = nullptr;
	vk_demo::DVKShader*				m_Shader0 = nullptr;
	vk_demo::DVKDescriptorSet*		m_DescriptorSet0 = nullptr;
	
	vk_demo::DVKGfxPipeline*        m_Pipeline1 = nullptr;
	vk_demo::DVKShader*				m_Shader1 = nullptr;

	vk_demo::DVKGfxPipeline*		m_Pipeline2 = nullptr;
	vk_demo::DVKShader*				m_Shader2 = nullptr;

	DVKDescriptorSetArray			m_DescriptorSets;

	DVKDescriptorSetArray           m_DebugDescriptorSets;

	DVKTextureArray					m_AttachsDepth;
	DVKTextureArray					m_AttachsColor;
    DVKTextureArray                 m_AttachsNormal;
	DVKTextureArray					m_AttachsPosition;
	DVKTextureArray					m_AttachsDebug;
	
	ImageGUIContext*				m_GUI = nullptr;
};

std::shared_ptr<AppModuleBase> CreateAppMode(const std::vector<std::string>& cmdLine)
{
	return std::make_shared<DeferredShadingDemo>(1400, 900, "DeferredShadingDemo", cmdLine);
}
