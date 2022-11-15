#include "skyBoxSphereGraphicsPipeline.h"
#include <glm/gtx/transform.hpp>
#include "initialization.h"
#include "guiControl.h"
#include "light.h"

SkyBoxSphereGraphicsPipeline::SkyBoxSphereGraphicsPipeline()
{
}

SkyBoxSphereGraphicsPipeline::~SkyBoxSphereGraphicsPipeline()
{
}

void SkyBoxSphereGraphicsPipeline::updateData(AppResources* app, GuiControl* gui_control)
{
	pushConstants.moonCol = Moon::color;
	pushConstants.sunCol = Sun::color;

	glm::vec3 cameraPos = gui_control->camera.getPosition();
	pushConstants.cameraPos = cameraPos;
	pushConstants.time = (gui_control->var.time - 12.0f) / 24.0f * 360.0f;

	glm::mat4 suntrans = glm::rotate(glm::radians(-30.0f), glm::normalize(glm::vec3(0.0f, 0.0f, 1.0f))) *
		glm::rotate(glm::radians(pushConstants.time), glm::normalize(glm::vec3(1.0f, 0.1f, 0.1f)));
	pushConstants.M = glm::translate(cameraPos) * suntrans;

	Sun::position = suntrans * glm::vec4(Sun::initialPosition, 1.0f);
	Sun::direction = glm::normalize(Sun::position);
	glm::mat4 sun_rot = pushConstants.M;
	pushConstants.sunPos = sun_rot * glm::vec4(Sun::initialPosition, 1.0f);

	glm::mat4 moontrans = glm::rotate(glm::radians(40.0f), glm::normalize(glm::vec3(1.0f, 0.0f, 0.0f))) *
		glm::rotate(glm::radians(pushConstants.time), glm::normalize(glm::vec3(0.4f, 0.1f, 1.0f)));
	glm::mat4 moon_rot =
		glm::translate(cameraPos) * moontrans;

	Moon::position = moontrans * glm::vec4(Moon::initialPosition, 1.0f);
	Moon::direction = glm::normalize(Moon::position);
	pushConstants.moonPos = moon_rot * glm::vec4(Moon::initialPosition, 1.0f);

	pushConstants.sunMoonSize = sun_moon_size;
	pushConstants.sunMoonImageSize = sun_moon_image_size;
	pushConstants.moonLuminance = moonLuminance;
	pushConstants.cloudVP =
		glm::ortho(-sun_moon_image_size, sun_moon_image_size, -sun_moon_image_size, sun_moon_image_size, 0.0f,
				   500.0f) * glm::lookAt(cameraPos, pushConstants.moonPos, glm::vec3(0.0f, 1.0f, 0.0f));
	pushConstants.M = sun_rot * glm::scale(glm::vec3(480.0f));
}

void SkyBoxSphereGraphicsPipeline::createPipeline(AppResources* app, bool full)
{
	vk::ShaderModule vertexShaderModule, fragmentShaderModule;
	createShaderModule(app->device, "skysphere.vert", vertexShaderModule);
	createShaderModule(app->device, "skysphere.frag", fragmentShaderModule);

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
	viewport.width = (float)skybox_cubemap_size;
	viewport.height = (float)skybox_cubemap_size;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	vk::Rect2D scissor = {};
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	scissor.extent.width = skybox_cubemap_size;
	scissor.extent.height = skybox_cubemap_size;

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
	multisampleCreateInfo.rasterizationSamples = vk::SampleCountFlagBits::e1;
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

	// Note: all attachments must have the same values unless a device feature is enabled
	vk::PipelineColorBlendStateCreateInfo colorBlendCreateInfo = {};
	colorBlendCreateInfo.logicOpEnable = VK_FALSE;
	colorBlendCreateInfo.logicOp = vk::LogicOp::eCopy;
	colorBlendCreateInfo.attachmentCount = 1;
	colorBlendCreateInfo.pAttachments = &colorBlendAttachmentState;
	colorBlendCreateInfo.blendConstants[0] = 0.0f;
	colorBlendCreateInfo.blendConstants[1] = 0.0f;
	colorBlendCreateInfo.blendConstants[2] = 0.0f;
	colorBlendCreateInfo.blendConstants[3] = 0.0f;

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
	pipelineCreateInfo.layout = pipelineLayout;
	pipelineCreateInfo.renderPass = *renderPass;
	pipelineCreateInfo.subpass = 0;
	pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineCreateInfo.basePipelineIndex = -1;

	pipeline = app->device.createGraphicsPipeline(nullptr, pipelineCreateInfo).value;

	app->device.destroyShaderModule(vertexShaderModule);
	app->device.destroyShaderModule(fragmentShaderModule);
}
