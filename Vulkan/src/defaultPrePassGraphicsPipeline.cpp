#include "defaultPrePassGraphicsPipeline.h"
#include "initialization.h"
#include "guiControl.h"
#include <glm/gtc/type_ptr.hpp>

DefaultPrePassGraphicsPipeline::DefaultPrePassGraphicsPipeline()
{
}

DefaultPrePassGraphicsPipeline::~DefaultPrePassGraphicsPipeline()
{
}

void DefaultPrePassGraphicsPipeline::setUniformBuffer(UniformBuffer* buffer)
{
	uniformBuffer = buffer;
}

void DefaultPrePassGraphicsPipeline::setMeshTransformBuffer(Buffer* _meshTransformBuffer)
{
	meshTransformBuffer = _meshTransformBuffer;
}

void DefaultPrePassGraphicsPipeline::setMeshNormalTransformBuffer(Buffer* _meshNormalTransformBuffer)
{
	meshNormalTransformBuffer = _meshNormalTransformBuffer;
}

void DefaultPrePassGraphicsPipeline::setTexturesAndMaterials(std::vector<SampledTexture>* _textures, Buffer* _materialBuffer)
{
	textures = _textures;
	materialBuffer = _materialBuffer;
}

void DefaultPrePassGraphicsPipeline::setLineVisualizationBuffers(Buffer* _drawIndirectLinesBuffer, Buffer* _linesBuffer)
{
	drawIndirectLinesBuffer = _drawIndirectLinesBuffer;
	linesBuffer = _linesBuffer;
}


void DefaultPrePassGraphicsPipeline::updateData(AppResources* app, GuiControl* gui_control)
{
	uniformBuffer->bufferData.VP = gui_control->camera.getVP(gui_control->var.jitter, app->frameCount);
	fillDeviceBuffer(app->device, uniformBuffer->buffer.mem, getUniformBufferData(*uniformBuffer));

	pushConstants.drawNormalMaps = (uint32_t)gui_control->var.drawNormalMaps;
	pushConstants.width = (uint32_t)app->swapChainExtent.width;
	pushConstants.height = (uint32_t)app->swapChainExtent.height;
	pushConstants.keepCurrentLines = gui_control->var.keepCurrentLines;
}

void DefaultPrePassGraphicsPipeline::cleanUpPipeline(AppResources* app, bool full)
{
	app->device.destroyPipelineLayout(pipelineLayout, nullptr);
	app->device.destroyPipeline(pipeline);

	if (full)
	{
		app->device.destroyDescriptorSetLayout(descriptorSetLayout);
		app->device.destroyDescriptorPool(descriptorPool);
	}
}

void DefaultPrePassGraphicsPipeline::bindPushConstants(vk::CommandBuffer* cb)
{
	cb->pushConstants(pipelineLayout,
					  vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
					  0, sizeof(PushStruct),
					  &pushConstants);
}

void DefaultPrePassGraphicsPipeline::createVertexInformation()
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

void DefaultPrePassGraphicsPipeline::createPipeline(AppResources* app, bool full)
{
	vk::ShaderModule vertexShaderModule, fragmentShaderModule;
	createShaderModule(app->device, "defaultprepass.vert", vertexShaderModule);
	createShaderModule(app->device, "defaultprepass.frag", fragmentShaderModule);

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
	rasterizationCreateInfo.cullMode = vk::CullModeFlagBits::eBack;
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
	depthStencilInfo.depthTestEnable = VK_TRUE;
	depthStencilInfo.depthWriteEnable = VK_TRUE;
	depthStencilInfo.depthCompareOp = vk::CompareOp::eLess;
	depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
	depthStencilInfo.minDepthBounds = 0.0f; // Optional
	depthStencilInfo.maxDepthBounds = 1.0f; // Optional
	depthStencilInfo.stencilTestEnable = VK_FALSE;

	if (full)
	{
		std::vector<vk::DescriptorSetLayoutBinding> bindings;
		bindings.emplace_back(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex);
		bindings.emplace_back(1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eFragment);
		bindings.emplace_back(2, vk::DescriptorType::eCombinedImageSampler, textures->size(), vk::ShaderStageFlagBits::eFragment);
		bindings.emplace_back(3, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eVertex);
		bindings.emplace_back(4, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eVertex);
		bindings.emplace_back(5, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eFragment);
		bindings.emplace_back(6, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eFragment);
		// TODO bind other useful buffers

		vk::DescriptorSetLayoutCreateInfo descriptorLayoutCreateInfo = {};
		descriptorLayoutCreateInfo.bindingCount = bindings.size();
		descriptorLayoutCreateInfo.pBindings = bindings.data();

		descriptorSetLayout = app->device.createDescriptorSetLayout(descriptorLayoutCreateInfo);
	}

	vk::PushConstantRange pcr(
		vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
		0, sizeof(PushStruct));
	vk::PipelineLayoutCreateInfo layoutCreateInfo = {};
	layoutCreateInfo.setLayoutCount = 1;
	layoutCreateInfo.pSetLayouts = &descriptorSetLayout;
	layoutCreateInfo.pushConstantRangeCount = 1;
	layoutCreateInfo.pPushConstantRanges = &pcr;

	pipelineLayout = app->device.createPipelineLayout(layoutCreateInfo);

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
	pipelineCreateInfo.renderPass = app->preRenderPass;
	pipelineCreateInfo.subpass = 0;
	pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineCreateInfo.basePipelineIndex = -1;
	pipelineCreateInfo.pDepthStencilState = &depthStencilInfo;

	pipeline = app->device.createGraphicsPipeline(nullptr, pipelineCreateInfo).value;

	app->device.destroyShaderModule(vertexShaderModule);
	app->device.destroyShaderModule(fragmentShaderModule);
}

void DefaultPrePassGraphicsPipeline::createDescriptorPool(AppResources* app)
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

void DefaultPrePassGraphicsPipeline::createDescriptorSet(AppResources* app)
{
	// There needs to be one descriptor set per binding point in the shader
	vk::DescriptorSetAllocateInfo allocInfo = {};
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &descriptorSetLayout;

	descriptorSet = app->device.allocateDescriptorSets(allocInfo)[0];

	std::vector<vk::WriteDescriptorSet> wds(7);

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

	vk::DescriptorBufferInfo materialDescriptorBufferInfo = {};
	materialDescriptorBufferInfo.buffer = materialBuffer->buf;
	materialDescriptorBufferInfo.offset = 0;
	materialDescriptorBufferInfo.range = VK_WHOLE_SIZE;
	wds[1].dstSet = descriptorSet;
	wds[1].descriptorCount = 1;
	wds[1].descriptorType = vk::DescriptorType::eStorageBuffer;
	wds[1].pBufferInfo = &materialDescriptorBufferInfo;
	wds[1].dstBinding = 1;

	std::vector<vk::DescriptorImageInfo> descriptorImageInfos(textures->size());
	for (size_t i = 0; i < textures->size(); ++i)
	{
		descriptorImageInfos[i].imageView = textures->at(i).imageView;
		descriptorImageInfos[i].imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		descriptorImageInfos[i].sampler = textures->at(i).sampler;
	}

	wds[2].dstSet = descriptorSet;
	wds[2].descriptorCount = descriptorImageInfos.size();;
	wds[2].descriptorType = vk::DescriptorType::eCombinedImageSampler;
	wds[2].pImageInfo = descriptorImageInfos.data();
	wds[2].dstBinding = 2;

	vk::DescriptorBufferInfo meshTransformDescriptorBufferInfo = {};
	meshTransformDescriptorBufferInfo.buffer = meshTransformBuffer->buf;
	meshTransformDescriptorBufferInfo.offset = 0;
	meshTransformDescriptorBufferInfo.range = VK_WHOLE_SIZE;
	wds[3].dstSet = descriptorSet;
	wds[3].descriptorCount = 1;
	wds[3].descriptorType = vk::DescriptorType::eStorageBuffer;
	wds[3].pBufferInfo = &meshTransformDescriptorBufferInfo;
	wds[3].dstBinding = 3;

	vk::DescriptorBufferInfo meshNormalTransformDescriptorBufferInfo = {};
	meshNormalTransformDescriptorBufferInfo.buffer = meshNormalTransformBuffer->buf;
	meshNormalTransformDescriptorBufferInfo.offset = 0;
	meshNormalTransformDescriptorBufferInfo.range = VK_WHOLE_SIZE;
	wds[4].dstSet = descriptorSet;
	wds[4].descriptorCount = 1;
	wds[4].descriptorType = vk::DescriptorType::eStorageBuffer;
	wds[4].pBufferInfo = &meshNormalTransformDescriptorBufferInfo;
	wds[4].dstBinding = 4;

	vk::DescriptorBufferInfo drawIndirectLinesDescriptorBufferInfo = {};
	drawIndirectLinesDescriptorBufferInfo.buffer = drawIndirectLinesBuffer->buf;
	drawIndirectLinesDescriptorBufferInfo.offset = 0;
	drawIndirectLinesDescriptorBufferInfo.range = VK_WHOLE_SIZE;
	wds[5].dstSet = descriptorSet;
	wds[5].descriptorCount = 1;
	wds[5].descriptorType = vk::DescriptorType::eStorageBuffer;
	wds[5].pBufferInfo = &drawIndirectLinesDescriptorBufferInfo;
	wds[5].dstBinding = 5;

	vk::DescriptorBufferInfo linesDescriptorBufferInfo = {};
	linesDescriptorBufferInfo.buffer = linesBuffer->buf;
	linesDescriptorBufferInfo.offset = 0;
	linesDescriptorBufferInfo.range = VK_WHOLE_SIZE;
	wds[6].dstSet = descriptorSet;
	wds[6].descriptorCount = 1;
	wds[6].descriptorType = vk::DescriptorType::eStorageBuffer;
	wds[6].pBufferInfo = &linesDescriptorBufferInfo;
	wds[6].dstBinding = 6;

	app->device.updateDescriptorSets(wds.size(), wds.data(), 0U, nullptr);
}

std::vector<float> DefaultPrePassGraphicsPipeline::getUniformBufferData(UniformBuffer& ub)
{
	std::vector<float> v;
	v.insert(v.end(), glm::value_ptr(ub.bufferData.VP), glm::value_ptr(ub.bufferData.VP) + 16);
	return v;
}
