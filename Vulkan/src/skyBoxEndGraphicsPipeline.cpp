#include "skyBoxEndGraphicsPipeline.h"
#include <glm/gtx/transform.hpp>
#include "initialization.h"
#include "guiControl.h"
#include "light.h"

SkyBoxEndGraphicsPipeline::SkyBoxEndGraphicsPipeline()
{
}

SkyBoxEndGraphicsPipeline::~SkyBoxEndGraphicsPipeline()
{
}

void SkyBoxEndGraphicsPipeline::updateData(AppResources* app, GuiControl* gui_control)
{
	glm::vec3 cameraPos = gui_control->camera.getPosition();
	pushConstants.M = gui_control->camera.getVP(gui_control->var.jitter, app->frameCount) * glm::translate(cameraPos);
	pushConstants.tonemapping = gui_control->var.toneMapping;
}

void SkyBoxEndGraphicsPipeline::createPipeline(AppResources* app, bool full)
{
	vk::ShaderModule vertexShaderModule, fragmentShaderModule;
	createShaderModule(app->device, "skyboxend.vert", vertexShaderModule);
	createShaderModule(app->device, "skyboxend.frag", fragmentShaderModule);

	// Set up shader stage info
	vk::PipelineShaderStageCreateInfo vertexShaderCreateInfo{};
	vertexShaderCreateInfo.stage = vk::ShaderStageFlagBits::eVertex;
	vertexShaderCreateInfo.module = vertexShaderModule;
	vertexShaderCreateInfo.pName = "main";

	vk::PipelineShaderStageCreateInfo fragmentShaderCreateInfo = {};
	fragmentShaderCreateInfo.stage = vk::ShaderStageFlagBits::eFragment;
	fragmentShaderCreateInfo.module = fragmentShaderModule;
	fragmentShaderCreateInfo.pName = "main";

	std::vector<vk::PipelineShaderStageCreateInfo> shaderStages = { vertexShaderCreateInfo, fragmentShaderCreateInfo };

	// Describe vertex input
	vk::PipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
	vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
	vertexInputCreateInfo.pVertexBindingDescriptions = &vertexBindingDescription;
	vertexInputCreateInfo.vertexAttributeDescriptionCount = vertexAttributeDescriptions.size();
	vertexInputCreateInfo.pVertexAttributeDescriptions = vertexAttributeDescriptions.data();

	// Describe input assembly
	vk::PipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {};
	inputAssemblyCreateInfo.topology = vk::PrimitiveTopology::eTriangleList;
	inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

	// Describe viewport and scissor
	vk::Viewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)app->swapChainExtent.width;
	viewport.height = (float)app->swapChainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	vk::Rect2D scissor = {};
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	scissor.extent.width = app->swapChainExtent.width;
	scissor.extent.height = app->swapChainExtent.height;

	// Note: scissor test is always enabled (although dynamic scissor is possible)
	// Number of viewports must match number of scissors
	vk::PipelineViewportStateCreateInfo viewportCreateInfo = {};
	viewportCreateInfo.viewportCount = 1;
	viewportCreateInfo.pViewports = &viewport;
	viewportCreateInfo.scissorCount = 1;
	viewportCreateInfo.pScissors = &scissor;

	// Describe rasterization
	// Note: depth bias and using polygon modes other than fill require changes to logical device creation (device features)
	vk::PipelineRasterizationStateCreateInfo rasterizationCreateInfo = {};
	rasterizationCreateInfo.depthClampEnable = VK_FALSE;
	rasterizationCreateInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterizationCreateInfo.polygonMode = vk::PolygonMode::eFill;
	rasterizationCreateInfo.cullMode = vk::CullModeFlagBits::eNone;
	rasterizationCreateInfo.frontFace = vk::FrontFace::eClockwise;
	rasterizationCreateInfo.depthBiasEnable = VK_FALSE;
	rasterizationCreateInfo.depthBiasConstantFactor = 0.0f;
	rasterizationCreateInfo.depthBiasClamp = 0.0f;
	rasterizationCreateInfo.depthBiasSlopeFactor = 0.0f;
	rasterizationCreateInfo.lineWidth = 1.0f;

	// Describe multisampling
	// Note: using multisampling also requires turning on device features
	vk::PipelineMultisampleStateCreateInfo multisampleCreateInfo = {};
	multisampleCreateInfo.rasterizationSamples = app->msaa;
	multisampleCreateInfo.sampleShadingEnable = VK_FALSE;
	multisampleCreateInfo.minSampleShading = 1.0f;
	multisampleCreateInfo.alphaToCoverageEnable = VK_FALSE;
	multisampleCreateInfo.alphaToOneEnable = VK_FALSE;

	// Describing color blending
	// Note: all paramaters except blendEnable and colorWriteMask are irrelevant here
	vk::PipelineColorBlendAttachmentState colorBlendAttachmentState = {};
	colorBlendAttachmentState.blendEnable = VK_TRUE;
	colorBlendAttachmentState.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
	colorBlendAttachmentState.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
	colorBlendAttachmentState.colorBlendOp = vk::BlendOp::eAdd;
	colorBlendAttachmentState.srcAlphaBlendFactor = vk::BlendFactor::eSrcAlpha;
	colorBlendAttachmentState.dstAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
	colorBlendAttachmentState.alphaBlendOp = vk::BlendOp::eAdd;
	colorBlendAttachmentState.colorWriteMask =
		vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB |
		vk::ColorComponentFlagBits::eA;

	vk::PipelineColorBlendAttachmentState positionBlendAttachmentState = {};
	positionBlendAttachmentState.blendEnable = VK_FALSE;
	positionBlendAttachmentState.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
	positionBlendAttachmentState.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
	positionBlendAttachmentState.colorBlendOp = vk::BlendOp::eAdd;
	positionBlendAttachmentState.srcAlphaBlendFactor = vk::BlendFactor::eSrcAlpha;
	positionBlendAttachmentState.dstAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
	positionBlendAttachmentState.alphaBlendOp = vk::BlendOp::eAdd;
	positionBlendAttachmentState.colorWriteMask =
		vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB |
		vk::ColorComponentFlagBits::eA;

	vk::PipelineColorBlendAttachmentState normalBlendAttachmentState = {};
	normalBlendAttachmentState.blendEnable = VK_FALSE;
	normalBlendAttachmentState.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
	normalBlendAttachmentState.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
	normalBlendAttachmentState.colorBlendOp = vk::BlendOp::eAdd;
	normalBlendAttachmentState.srcAlphaBlendFactor = vk::BlendFactor::eSrcAlpha;
	normalBlendAttachmentState.dstAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
	normalBlendAttachmentState.alphaBlendOp = vk::BlendOp::eAdd;
	normalBlendAttachmentState.colorWriteMask =
		vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB |
		vk::ColorComponentFlagBits::eA;

	vk::PipelineColorBlendAttachmentState tangentBlendAttachmentState = {};
	tangentBlendAttachmentState.blendEnable = VK_FALSE;
	tangentBlendAttachmentState.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
	tangentBlendAttachmentState.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
	tangentBlendAttachmentState.colorBlendOp = vk::BlendOp::eAdd;
	tangentBlendAttachmentState.srcAlphaBlendFactor = vk::BlendFactor::eSrcAlpha;
	tangentBlendAttachmentState.dstAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
	tangentBlendAttachmentState.alphaBlendOp = vk::BlendOp::eAdd;
	tangentBlendAttachmentState.colorWriteMask =
		vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB |
		vk::ColorComponentFlagBits::eA;

	vk::PipelineColorBlendAttachmentState bitangentBlendAttachmentState = {};
	bitangentBlendAttachmentState.blendEnable = VK_FALSE;
	bitangentBlendAttachmentState.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
	bitangentBlendAttachmentState.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
	bitangentBlendAttachmentState.colorBlendOp = vk::BlendOp::eAdd;
	bitangentBlendAttachmentState.srcAlphaBlendFactor = vk::BlendFactor::eSrcAlpha;
	bitangentBlendAttachmentState.dstAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
	bitangentBlendAttachmentState.alphaBlendOp = vk::BlendOp::eAdd;
	bitangentBlendAttachmentState.colorWriteMask =
		vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB |
		vk::ColorComponentFlagBits::eA;

	std::vector<vk::PipelineColorBlendAttachmentState> attachmentstates = {
		colorBlendAttachmentState,
		positionBlendAttachmentState,
		normalBlendAttachmentState,
		tangentBlendAttachmentState,
		bitangentBlendAttachmentState
	};
	// Note: all attachments must have the same values unless a device feature is enabled
	vk::PipelineColorBlendStateCreateInfo colorBlendCreateInfo = {};
	colorBlendCreateInfo.logicOpEnable = VK_FALSE;
	colorBlendCreateInfo.logicOp = vk::LogicOp::eCopy;
	colorBlendCreateInfo.attachmentCount = attachmentstates.size();
	colorBlendCreateInfo.pAttachments = attachmentstates.data();
	colorBlendCreateInfo.blendConstants[0] = 0.0f;
	colorBlendCreateInfo.blendConstants[1] = 0.0f;
	colorBlendCreateInfo.blendConstants[2] = 0.0f;
	colorBlendCreateInfo.blendConstants[3] = 0.0f;

	// Depth and stencil state
	vk::PipelineDepthStencilStateCreateInfo depthStencilInfo = {};
	depthStencilInfo.depthTestEnable = VK_FALSE;
	depthStencilInfo.depthWriteEnable = VK_FALSE;
	depthStencilInfo.depthCompareOp = vk::CompareOp::eLess;
	depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
	depthStencilInfo.minDepthBounds = 0.0f; // Optional
	depthStencilInfo.maxDepthBounds = 1.0f; // Optional
	depthStencilInfo.stencilTestEnable = VK_FALSE;
	//depthStencilInfo.front = {}; // Optional
	//depthStencilInfo.back = {}; // Optional

	if (full)
	{
		std::vector<vk::DescriptorSetLayoutBinding> skyboxBindings;
		skyboxBindings.emplace_back(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex);
		skyboxBindings.emplace_back(1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment);
		skyboxBindings.emplace_back(2, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment);
		skyboxBindings.emplace_back(3, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment);

		vk::DescriptorSetLayoutCreateInfo skyBoxDescriptorLayoutCreateInfo = {};
		skyBoxDescriptorLayoutCreateInfo.bindingCount = skyboxBindings.size();
		skyBoxDescriptorLayoutCreateInfo.pBindings = skyboxBindings.data();

		descriptorSetLayout = app->device.createDescriptorSetLayout(skyBoxDescriptorLayoutCreateInfo);
	}

	vk::PushConstantRange skyboxPCR(
		vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
		0, sizeof(PushStruct));
	vk::PipelineLayoutCreateInfo skyboxLayoutCreateInfo = {};
	skyboxLayoutCreateInfo.setLayoutCount = 1;
	skyboxLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;
	skyboxLayoutCreateInfo.pushConstantRangeCount = 1;
	skyboxLayoutCreateInfo.pPushConstantRanges = &skyboxPCR;

	pipelineLayout = app->device.createPipelineLayout(skyboxLayoutCreateInfo);

	// Create the graphics pipeline
	vk::GraphicsPipelineCreateInfo pipelineCreateInfo = {};
	pipelineCreateInfo.stageCount = 2;
	pipelineCreateInfo.pStages = shaderStages.data();
	pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
	pipelineCreateInfo.pViewportState = &viewportCreateInfo;
	pipelineCreateInfo.pRasterizationState = &rasterizationCreateInfo;
	pipelineCreateInfo.pMultisampleState = &multisampleCreateInfo;
	pipelineCreateInfo.pColorBlendState = &colorBlendCreateInfo;
	pipelineCreateInfo.pDepthStencilState = &depthStencilInfo;
	pipelineCreateInfo.layout = pipelineLayout;
	pipelineCreateInfo.renderPass = *renderPass;
	pipelineCreateInfo.subpass = 0;
	pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineCreateInfo.basePipelineIndex = -1;

	pipeline = app->device.createGraphicsPipeline(nullptr, pipelineCreateInfo).value;

	app->device.destroyShaderModule(vertexShaderModule);
	app->device.destroyShaderModule(fragmentShaderModule);
}

void SkyBoxEndGraphicsPipeline::bindPushConstants(vk::CommandBuffer* cb)
{
	cb->pushConstants(pipelineLayout,
					  vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
					  0, sizeof(PushStruct),
					  &pushConstants);
}
