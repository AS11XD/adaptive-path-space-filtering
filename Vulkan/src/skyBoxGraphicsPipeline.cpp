#include "skyBoxGraphicsPipeline.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>
#include "initialization.h"
#include "guiControl.h"

SkyBoxGraphicsPipeline::SkyBoxGraphicsPipeline()
{
}

SkyBoxGraphicsPipeline::~SkyBoxGraphicsPipeline()
{
}

void SkyBoxGraphicsPipeline::setUniformBuffer(UniformBuffer* buffer)
{
	uniformBuffer = buffer;
}

void SkyBoxGraphicsPipeline::setRenderPass(vk::RenderPass* _renderPass)
{
	renderPass = _renderPass;
}

void SkyBoxGraphicsPipeline::setSampledTextures(SampledTexture* _skyBoxTex, SampledTexture* _moonTex, SampledTexture* _renderedCubemap)
{
	skyboxTex = _skyBoxTex;
	moonTex = _moonTex;
	renderedCubemap = _renderedCubemap;
}

void SkyBoxGraphicsPipeline::updateData(AppResources* app, GuiControl* gui_control)
{
	glm::vec3 position = gui_control->camera.getPosition();
	glm::mat4 proj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 1000.0f);

	uniformBuffer->bufferData.matrices[0] = proj * glm::lookAt(position, position + glm::vec3(1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0));
	uniformBuffer->bufferData.matrices[1] = proj * glm::lookAt(position, position + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0));
	uniformBuffer->bufferData.matrices[2] = proj * glm::lookAt(position, position + glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0));
	uniformBuffer->bufferData.matrices[3] = proj * glm::lookAt(position, position + glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, 0.0, -1.0));
	uniformBuffer->bufferData.matrices[4] = proj * glm::lookAt(position, position + glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, -1.0, 0.0));
	uniformBuffer->bufferData.matrices[5] = proj * glm::lookAt(position, position + glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, -1.0, 0.0));
	fillDeviceBuffer(app->device, uniformBuffer->buffer.mem, getUniformBufferData(*uniformBuffer));

	glm::vec3 cameraPos = gui_control->camera.getPosition();
	pushConstants.cameraPos = cameraPos;
	pushConstants.time = (gui_control->var.time - 12.0f) / 24.0f * 360.0f;

	pushConstants.M =
		glm::translate(cameraPos) * glm::rotate(glm::radians(-30.0f), glm::normalize(glm::vec3(0.0f, 0.0f, 1.0f))) *
		glm::rotate(glm::radians(pushConstants.time), glm::normalize(glm::vec3(1.0f, 0.1f, 0.1f)));
}

void SkyBoxGraphicsPipeline::cleanUpPipeline(AppResources* app, bool full)
{
	app->device.destroyPipelineLayout(pipelineLayout, nullptr);
	app->device.destroyPipeline(pipeline);

	if (full)
	{
		app->device.destroyDescriptorSetLayout(descriptorSetLayout);
		app->device.destroyDescriptorPool(descriptorPool);
	}
}

void SkyBoxGraphicsPipeline::bindPushConstants(vk::CommandBuffer* cb)
{
	cb->pushConstants(pipelineLayout,
					  vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
					  0, sizeof(PushStruct),
					  &pushConstants);
}

void SkyBoxGraphicsPipeline::createVertexInformation()
{
	// Binding and attribute descriptions
	vertexBindingDescription.binding = 0;
	vertexBindingDescription.stride = sizeof(float) * 15;
	vertexBindingDescription.inputRate = vk::VertexInputRate::eVertex;

	vertexAttributeDescriptions.resize(6);
	vertexAttributeDescriptions[0].binding = 0;
	vertexAttributeDescriptions[0].location = 0;
	vertexAttributeDescriptions[0].format = vk::Format::eR32G32B32Sfloat;

	vertexAttributeDescriptions[1].binding = 0;
	vertexAttributeDescriptions[1].location = 1;
	vertexAttributeDescriptions[1].format = vk::Format::eR32G32B32Sfloat;
	vertexAttributeDescriptions[1].offset = sizeof(float) * 3;

	vertexAttributeDescriptions[2].binding = 0;
	vertexAttributeDescriptions[2].location = 2;
	vertexAttributeDescriptions[2].format = vk::Format::eR32G32Sfloat;
	vertexAttributeDescriptions[2].offset = sizeof(float) * 6;

	vertexAttributeDescriptions[3].binding = 0;
	vertexAttributeDescriptions[3].location = 3;
	vertexAttributeDescriptions[3].format = vk::Format::eR32Uint;
	vertexAttributeDescriptions[3].offset = sizeof(float) * 8;

	vertexAttributeDescriptions[4].binding = 0;
	vertexAttributeDescriptions[4].location = 4;
	vertexAttributeDescriptions[4].format = vk::Format::eR32G32B32Sfloat;
	vertexAttributeDescriptions[4].offset = sizeof(float) * 9;

	vertexAttributeDescriptions[5].binding = 0;
	vertexAttributeDescriptions[5].location = 5;
	vertexAttributeDescriptions[5].format = vk::Format::eR32G32B32Sfloat;
	vertexAttributeDescriptions[5].offset = sizeof(float) * 12;
}

void SkyBoxGraphicsPipeline::createPipeline(AppResources* app, bool full)
{
	vk::ShaderModule vertexShaderModule, fragmentShaderModule;
	createShaderModule(app->device, "skybox.vert", vertexShaderModule);
	createShaderModule(app->device, "skybox.frag", fragmentShaderModule);

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

void SkyBoxGraphicsPipeline::createDescriptorPool(AppResources* app)
{
	// This describes how many descriptor sets we'll create from this pool for each type
	vk::DescriptorPoolSize typeCount;
	typeCount.descriptorCount = 1;
	vk::DescriptorPoolCreateInfo createInfo = {};
	createInfo.poolSizeCount = 1;
	createInfo.pPoolSizes = &typeCount;
	createInfo.maxSets = 1;

	descriptorPool = app->device.createDescriptorPool(createInfo);
}

void SkyBoxGraphicsPipeline::createDescriptorSet(AppResources* app)
{
	// There needs to be one descriptor set per binding point in the shader
	vk::DescriptorSetAllocateInfo allocInfo = {};
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &descriptorSetLayout;

	descriptorSet = app->device.allocateDescriptorSets(allocInfo)[0];

	std::vector<vk::WriteDescriptorSet> wds(4);

	// uniform buffer

	vk::DescriptorBufferInfo descriptorBufferInfo = {};
	descriptorBufferInfo.buffer = uniformBuffer->buffer.buf;
	descriptorBufferInfo.offset = 0;
	descriptorBufferInfo.range = VK_WHOLE_SIZE;
	wds[0].dstSet = descriptorSet;
	wds[0].descriptorCount = 1;
	wds[0].descriptorType = vk::DescriptorType::eUniformBuffer;
	wds[0].pBufferInfo = &descriptorBufferInfo;
	wds[0].dstBinding = 0;

	// textures

	vk::DescriptorImageInfo skybox_image_info = {};
	skybox_image_info.imageView = skyboxTex->imageView;
	skybox_image_info.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	skybox_image_info.sampler = skyboxTex->sampler;

	wds[1].descriptorCount = 1;
	wds[1].descriptorType = vk::DescriptorType::eCombinedImageSampler;
	wds[1].pImageInfo = &skybox_image_info;
	wds[1].dstBinding = 1;
	wds[1].dstSet = descriptorSet;

	vk::DescriptorImageInfo moon_image_info = {};
	moon_image_info.imageView = moonTex->imageView;
	moon_image_info.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	moon_image_info.sampler = moonTex->sampler;

	vk::WriteDescriptorSet mwDescSet = {};
	wds[2].descriptorCount = 1;
	wds[2].descriptorType = vk::DescriptorType::eCombinedImageSampler;
	wds[2].pImageInfo = &moon_image_info;
	wds[2].dstBinding = 2;
	wds[2].dstSet = descriptorSet;

	vk::DescriptorImageInfo skybox_end_image_info = {};
	skybox_end_image_info.imageView = renderedCubemap->imageView;
	skybox_end_image_info.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	skybox_end_image_info.sampler = renderedCubemap->sampler;

	vk::WriteDescriptorSet sbewDescSet = {};
	wds[3].descriptorCount = 1;
	wds[3].descriptorType = vk::DescriptorType::eCombinedImageSampler;
	wds[3].pImageInfo = &skybox_end_image_info;
	wds[3].dstBinding = 3;
	wds[3].dstSet = descriptorSet;

	app->device.updateDescriptorSets(wds.size(), wds.data(), 0U, nullptr);
}

std::vector<float> SkyBoxGraphicsPipeline::getUniformBufferData(UniformBuffer& ub)
{
	std::vector<float> v;
	v.insert(v.end(), glm::value_ptr(ub.bufferData.matrices[0]), glm::value_ptr(ub.bufferData.matrices[5]) + 16);
	return v;
}
